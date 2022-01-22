#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts>
#include <iostream>

using namespace std;

void MainWindow::DisplayLineChart()
{
    QLineSeries *series = new QLineSeries();
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
    this->DisplayLineChart();

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

