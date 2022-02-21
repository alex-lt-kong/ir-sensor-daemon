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
#include <stdio.h>

using namespace std;

int MainWindow::GetDBFromSFTP()
{
    QSettings settings("ak-studio", "ir-sensor-monitor");
    // ~/.config/ak-studio/ir-sensor-monitor.conf
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    FILE *sftpFilePtr = fopen((this->tempDir.path().toStdString() + "/" + this->dbName.c_str()).c_str(), "wb");
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

time_t MainWindow::DateStringToSecSinceEpoch(const char *time_str)
{
    struct tm tm = { 0 };

    if (!strptime(time_str, "%Y-%m-%d", &tm))
        return (time_t)-1;

    return mktime(&tm) - timezone;
}


int MainWindow::ReadDataFromDB()
{
    sqlite3* dbPtr;

    int retval = sqlite3_open((this->tempDir.path().toStdString() + "/" + this->dbName.c_str()).c_str(), &dbPtr);
    if (retval != SQLITE_OK) {
        ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "SQLite3 error: " + sqlite3_errmsg(dbPtr) + "\n");
        return (retval);
    }
    string query = "SELECT DATE(timestamp) AS 'Date', COUNT(*) AS 'RecordCount' "
                   "FROM DetectionRecords "
                   "GROUP BY DATE(timestamp)";

    sqlite3_stmt *stmt;
    retval = sqlite3_prepare_v2(dbPtr, query.c_str(), -1, &stmt, NULL);
    if (retval != SQLITE_OK) {
        ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "SQLite3 error: " + sqlite3_errmsg(dbPtr) + "\n");
        sqlite3_close(dbPtr);
        return (retval);
    }

    seriesOriginal->clear();
    seriesOriginal->setName("Raw Value/原始值");
    seriesMA->clear();
    seriesMA->setName("Moving Average/移动平均");

    int count = 0;
    float totalVal = 0;
    while ((retval = sqlite3_step(stmt)) == SQLITE_ROW) {
        count ++;
        const unsigned char *date = sqlite3_column_text(stmt, 0);
        // `unsigned char` is a character datatype where the variable consumes all the 8
        // bits of the memory and there is no sign bit (which is there in signed char);
        // `char` may be unsigned or signed depending on your platform;
        // In C, `char` is just another integral type
        float val = sqlite3_column_int(stmt, 1);
        totalVal += val;
        seriesOriginal->append(DateStringToSecSinceEpoch((char*)date) * 1000, val);
        seriesMA->append(DateStringToSecSinceEpoch((char*)date) * 1000, totalVal / count);

        if (minVal > val) minVal = val;
        if (maxVal < val) maxVal = val;
        // The series must have at least two data points for the plotting function to work.
    }
    if (retval != SQLITE_DONE) {
        ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "SQLite3 error: " + sqlite3_errmsg(dbPtr) + "\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(dbPtr);

    return retval;
}

void MainWindow::DisplayLineChart()
{
    while (chart->series().length() > 0) {
        chart->removeSeries(chart->series().at(0));
        // chart->series()[0] is functionally okay, but results
        // in the warning "Don't call QList::operator[] on temporary"
        // https://stackoverflow.com/questions/48876448/dont-call-qstringoperator-on-temporary/48881232
    }
    while (chart->axes().length() > 0) {
        chart->removeAxis(chart->axes().at(0));
    }

    chart->addSeries(seriesMA);
    chart->addSeries(seriesOriginal);
    chart->setMargins(QMargins(0,0,0,0));
    chart->setTitle("Detection Count by Date/每日侦测数");

    axisX->setTickCount(7);
    axisX->setFormat("yyyy-MM-dd");
    axisX->setTitleText("Date/日期");
    chart->addAxis(axisX, Qt::AlignBottom);
    seriesOriginal->attachAxis(axisX);
    seriesMA->attachAxis(axisX);

    axisY->setLabelFormat("%i");
    axisY->setTitleText("Detection Count/侦测数");
    axisY->setMin(minVal * 0.5);
    axisY->setMax(maxVal * 1.1);
    this->chart->addAxis(axisY, Qt::AlignLeft);
    seriesOriginal->attachAxis(axisY);
    seriesMA->attachAxis(axisY);

    // this graphicsView is "promoted to" a QChartView
    // https://stackoverflow.com/questions/48362864/how-to-insert-qchartview-in-form-with-qt-designer
    this->ui->graphicsView->setChart(this->chart);
    this->ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    this->ui->graphicsView->setRubberBand(QChartView::NoRubberBand);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    if (this->tempDir.isValid() == false) {
        this->ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "tempDir is invalid: " + this->tempDir.errorString() + "\n");
    }
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
    ui->pushButtonExit->setEnabled(false);
    qApp->processEvents();

    this->ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "Fetching latest database file from remote to " + this->tempDir.path() + "...\n");
    this->ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum());
    qApp->processEvents();

    this->GetDBFromSFTP();
    this->ui->plainTextEdit->insertPlainText(this->GetFormattedDateTime() + "Done\n");
    qApp->processEvents();

    this->ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum() - 1);

    this->ReadDataFromDB();
    this->DisplayLineChart();
    this->ui->pushButtonLoad->setEnabled(true);
    ui->pushButtonExit->setEnabled(true);
}


void MainWindow::on_pushButtonExit_clicked()
{
    QApplication::quit();
}

