#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts>
#include <iostream>
#include <sqlite3.h>
#include <curl/curl.h>
#include <QApplication>
#include <QThread>

using namespace std;

struct FtpFile {
  const char *filename;
  FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb,
                        void *stream)
{
  struct FtpFile *out = (struct FtpFile *)stream;
  if(!out->stream) {
    /* open file for writing */
    out->stream = fopen(out->filename, "wb");
    if(!out->stream)
      return -1; /* failure, cannot open file to write */
  }
  return fwrite(buffer, size, nmemb, out->stream);
}


int MainWindow::getDBFromSFTP()
{
    QSettings settings("ak-studio", "ir-sensor-monitor");
    // ~/.config/ak-studio/ir-sensor-monitor.conf
    CURL *curl;
    CURLcode res;
    struct FtpFile ftpfile = {
      "detection-records.sqlite", /* name to store the file as if successful */
      NULL
    };

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL, settings.value("SFTP_URI").toString().toStdString().c_str());
      /* Define our callback to get called when there's data to be written */
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
      /* Set a pointer to our struct to pass to the callback */
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);
      curl_easy_setopt(curl, CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, settings.value("MD5").toString().toStdString().c_str());
      curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PUBLICKEY);
      // the public/private key pair has to be in PEM format
      // If you use ssh-keygen to generate the key pair, it may NOT in PEM format
      // by default!
      curl_easy_setopt(curl, CURLOPT_SSH_PUBLIC_KEYFILE , settings.value("SFTP_public_key").toString().toStdString().c_str());
      curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, settings.value("SFTP_private_key").toString().toStdString().c_str());

      /* Switch on full protocol/debug output */
      curl_easy_setopt(curl, CURLOPT_VERBOSE, this->ui->checkBoxVerboseLogging->isChecked() ? 1L : 0);

      res = curl_easy_perform(curl);

      /* always cleanup */
      curl_easy_cleanup(curl);

      if(CURLE_OK != res) {
        /* we failed */
        cerr << "Error: cURL statue code: " << res << endl;
      }
    }

    if(ftpfile.stream)
      fclose(ftpfile.stream); /* close the local file */

    curl_global_cleanup();

    return 0;
}

int MainWindow::ReadDataFromDB(QLineSeries *series, int &minVal, int &maxVal)
{
    sqlite3* dbPtr;
    int retval = sqlite3_open("./detection-records.sqlite", &dbPtr);
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
        delete[] year;
        delete[] month;
        delete[] day;
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
    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(series);
    chart->setTitle("Detection Count by Date");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(3);
    axisX->setFormat("yyyy-MM-dd");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Detection Count");
    axisY->setMin(minVal * 0.5);
    axisY->setMax(maxVal * 1.1);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // this graphicsView is "promoted to" a QChartView
    // https://stackoverflow.com/questions/48362864/how-to-insert-qchartview-in-form-with-qt-designer
    this->ui->graphicsView->setChart(chart);

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonLoad_clicked()
{
    this->ui->pushButtonLoad->setEnabled(false);
    this->ui->plainTextEdit->insertPlainText("Fetching database file from remote...\n");
    qApp->processEvents();
    this->getDBFromSFTP();
    this->ui->plainTextEdit->insertPlainText("Done\n");
    QLineSeries *series = new QLineSeries();
    int minVal = 2147483647, maxVal= 0;
    this->ReadDataFromDB(series, minVal, maxVal);
    this->DisplayLineChart(series, minVal, maxVal);
    this->ui->pushButtonLoad->setEnabled(true);
}

