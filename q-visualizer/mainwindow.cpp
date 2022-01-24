#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts>
#include <iostream>
#include <sqlite3.h>

using namespace std;



int MainWindow::ReadDataFromDB(QLineSeries *series, int &minVal, int &maxVal)
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
    int minVal = 2147483647, maxVal= 0;
    this->ReadDataFromDB(series, minVal, maxVal);
    this->DisplayLineChart(series, minVal, maxVal);

}

MainWindow::~MainWindow()
{
    delete ui;
}
