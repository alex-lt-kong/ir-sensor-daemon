#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts>
#include <iostream>
#include <sqlite3.h>

using namespace std;



int MainWindow::ReadDataFromDB(QLineSeries *series)
{
    sqlite3* dbPtr;
    int retval = sqlite3_open("./../detection-records.sqlite", &dbPtr);
    if (retval != SQLITE_OK) {
        cerr << "Unable to open the DB: " << sqlite3_errmsg(dbPtr) << endl;
        return (retval);
    }
    string query = "SELECT DATE(timestamp) AS 'Date', COUNT(*) AS 'RecordCount' "
                   "FROM DetectionRecords "
                   "GROUP BY DATE(timestamp)";

    /*
    Signature of
    int sqlite3_exec(
      sqlite3*,                                  // An open database
      const char *sql,                           // SQL to be evaluated
      int (*callback)(void*,int,char**,char**),  // Callback function
      void *,                                    // 1st argument to callback
      char **errmsg                              // Error msg written here
    );
    If the 5th parameter to sqlite3_exec() is not NULL then any error message is written into
    memory obtained from sqlite3_malloc() and passed back through the 5th parameter. To avoid
    memory leaks, the application should invoke sqlite3_free() on error message strings returned
    through the 5th parameter of sqlite3_exec() after the error message string is no longer needed.
    */
    retval = sqlite3_exec(dbPtr, query.c_str(), OnRowFetched, NULL, NULL);

    if(retval != SQLITE_OK ) {
        cerr <<"SQL error: " << sqlite3_errmsg(dbPtr) << endl;
        sqlite3_close(dbPtr);
        return retval;
    }
    return 0;
}

void MainWindow::DisplayLineChart(QLineSeries *series)
{
    QDateTime momentInTime;
    momentInTime.setDate(QDate(2020,1,1));
    series->append(momentInTime.toMSecsSinceEpoch(), 12);
    QDateTime momentInTime1;
    momentInTime1.setDate(QDate(2020,1,3));
    series->append(momentInTime1.toMSecsSinceEpoch(), 15);
    QDateTime momentInTime2;
    momentInTime2.setDate(QDate(2020,1,5));
    series->append(momentInTime2.toMSecsSinceEpoch(), -5);

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(series);
    chart->setTitle("Simple line chart example");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(3);
    axisX->setFormat("yyyy-MM-dd");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Sunspots count");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    this->setCentralWidget(chartView);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QLineSeries *series = new QLineSeries();
    this->ReadDataFromDB(series);
    this->DisplayLineChart(series);

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
   // cout << "clicked!" << endl;
    //this->DisplayLineChart();
}

