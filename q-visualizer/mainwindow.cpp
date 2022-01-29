#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <chrono>
#include <curl/curl.h>
#include <QtCharts>
#include <iostream>
#include <QApplication>
#include <QThread>
#include <qprocess.h>
#include <sqlite3.h>
#include <time.h>

using namespace std;

int MainWindow::GetDBFromSFTP()
{
    QSettings settings("ak-studio", "ir-sensor-monitor");
    // ~/.config/ak-studio/ir-sensor-monitor.conf
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    FILE *sftpFilePtr = fopen(this->dbName.c_str(), "wb");
    if (sftpFilePtr == NULL) {
        this->ui->plainTextEdit->insertPlainText(("Failed to open database file " + this->dbName + "\n").c_str());
        return 1;
    }
    curl = curl_easy_init();
    if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL, settings.value("SFTP_URI").toString().toStdString().c_str());
      //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataToFile);
      // WriteDataToFile() gets called by libcurl as soon as there is data received that needs to be saved.
      // For most transfers, this callback gets called many times and each invoke delivers another chunk of data.
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, sftpFilePtr);
      // ftpfile: pointer that points to a struct used by WriteDataToFile to
      curl_easy_setopt(curl, CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, settings.value("MD5").toString().toStdString().c_str());
      curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PUBLICKEY);
      // the public/private key pair has to be in PEM format
      // If you use ssh-keygen to generate the key pair, it may NOT in PEM format
      // by default!
      curl_easy_setopt(curl, CURLOPT_SSH_PUBLIC_KEYFILE , settings.value("SFTP_public_key").toString().toStdString().c_str());
      curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, settings.value("SFTP_private_key").toString().toStdString().c_str());
      curl_easy_setopt(curl, CURLOPT_VERBOSE, this->ui->checkBoxVerboseLogging->isChecked() ? 1L : 0);

      res = curl_easy_perform(curl);

      // curl_easy_init() call MUST have a corresponding call to
      // curl_easy_cleanup when the operation is complete.
      curl_easy_cleanup(curl);

      if(CURLE_OK != res) {
        this->ui->plainTextEdit->insertPlainText("Error: cURL statue code: " + QString::number(res) + "\n");
      }
    } else {
        this->ui->plainTextEdit->insertPlainText("Failed to initilize a cURL instance\n");
        return 1;
    }
    fclose (sftpFilePtr);

    curl_global_cleanup();

    return 0;
}

int MainWindow::ReadDataFromDB(QLineSeries *series, int &minVal, int &maxVal)
{
    sqlite3* dbPtr;
    int retval = sqlite3_open("detection-records.sqlite", &dbPtr);
    if (retval != SQLITE_OK) {
        cerr << "Unable to open the DB: " << sqlite3_errmsg(dbPtr) << endl;
        return (retval);
    }
    string query = "SELECT DATE(timestamp) AS 'Date', COUNT(*) AS 'RecordCount' "
                   "FROM DetectionRecords "
                   "GROUP BY DATE(timestamp)";

    sqlite3_stmt *stmt;
    retval = sqlite3_prepare_v2(dbPtr, query.c_str(), -1, &stmt, NULL);
    if (retval != SQLITE_OK) {
        cerr << "error: " << sqlite3_errmsg(dbPtr) << endl;
        sqlite3_close(dbPtr);
        return (retval);
    }
    while ((retval = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *date = sqlite3_column_text(stmt, 0);
        // `unsigned char` is a character datatype where the variable consumes all the 8
        // bits of the memory and there is no sign bit (which is there in signed char);
        // `char` may be unsigned or signed depending on your platform;
        // In C, `char` is just another integral type
        int count = sqlite3_column_int(stmt, 1);
        char* year = new char[4];
        char* month = new char[2];
        char* day = new char[2];
        memcpy(year, date, 4);
        memcpy(month, date+5, 2);
        memcpy(day, date+8, 2);
        QDateTime momentInTime;
        momentInTime.setDate(QDate(atoi(year), atoi(month), atoi(day)));
        series->append(momentInTime.toMSecsSinceEpoch(), count);
        if (minVal > count) minVal = count;
        if (maxVal < count) maxVal = count;
        //delete[] year;
        //delete[] month;
        //delete[] day;
        // seems cannot delete[] year,month,day...
        // The series must have at least two data points for the plotting function to work.
    }
    if (retval != SQLITE_DONE) {
        cerr << "error: " << sqlite3_errmsg(dbPtr) << endl;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(dbPtr);

    return retval;
}

void MainWindow::DisplayLineChart(QLineSeries *series, int minVal, int maxVal)
{
    QChart *oldChart = this->chart;
    // The logic here is a bit confusing: we first create a new pointer to
    // store the address of the old chart object, then we recreate a new
    // chart object to render new line chart. AFTER we call graphicsView->setChart(this->chart);
    // we can safely deallocate old chart's memory...

    this->chart = new QChart();
    this->chart->legend()->hide();
    this->chart->addSeries(series);
    this->chart->setMargins(QMargins(0,0,0,0));
    this->chart->setTitle("Detection Count by Date");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(3);
    axisX->setFormat("yyyy-MM-dd");
    axisX->setTitleText("Date");
    this->chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Detection Count");
    axisY->setMin(minVal * 0.5);
    axisY->setMax(maxVal * 1.1);
    this->chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // this graphicsView is "promoted to" a QChartView
    // https://stackoverflow.com/questions/48362864/how-to-insert-qchartview-in-form-with-qt-designer
    this->ui->graphicsView->setChart(this->chart);
    this->ui->graphicsView->setRubberBand(QChartView::NoRubberBand);

    delete oldChart;
    // The C++ standard does not specify any relation between new/delete and the
    // C memory allocation routines, but new and delete are typically implemented
    // as wrappers around malloc and free

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

QString MainWindow::GetFormattedDateTime() {

    time_t now;
    time(&now);
    char timeBuffer[sizeof("[1970-01-01 00:00:00] ")];
    strftime(timeBuffer, sizeof timeBuffer,"[%F %T] ", localtime(&now));
    return QString(timeBuffer);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonLoad_clicked()
{
    this->ui->pushButtonLoad->setEnabled(false);
    qApp->processEvents();

    if(rename(this->dbName.c_str(), (this->dbName + ".bak").c_str()) < 0) {
        ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "Failed moving existing database to ");
        ui->plainTextEdit->insertPlainText((this->dbName + ".bak: " + strerror(errno) + "\n").c_str());
    } else {
        ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "Moved existing database to ");
        ui->plainTextEdit->insertPlainText((this->dbName + ".bak\n").c_str());
    }
    qApp->processEvents();

    this->ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "Fetching latest database file from remote...\n");
    qApp->processEvents();
    this->GetDBFromSFTP();
    this->ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "Done\n");
    qApp->processEvents();
    this->ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum() - 1);
    QLineSeries *series = new QLineSeries();
    int minVal = 2147483647, maxVal= 0;
    this->ReadDataFromDB(series, minVal, maxVal);
    this->DisplayLineChart(series, minVal, maxVal);
    this->ui->pushButtonLoad->setEnabled(true);
}

