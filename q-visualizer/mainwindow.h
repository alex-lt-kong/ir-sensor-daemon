#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <QtCharts>
#include <curl/curl.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using namespace std;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void DisplayLineChart();
    int ReadDataFromDB();
    int GetDBFromSFTP();
    time_t DateStringToSecSinceEpoch(const char *time_str);

private slots:
     void on_pushButtonLoad_clicked();

     void on_pushButtonExit_clicked();

private:
    QLineSeries *seriesOriginal = new QLineSeries();
    int minVal = 2147483647;
    int maxVal = 0;
    QSplineSeries *seriesMA = new QSplineSeries();
    Ui::MainWindow *ui;
    QChart *chart = new QChart();
    QDateTimeAxis *axisX = new QDateTimeAxis;
    QValueAxis *axisY = new QValueAxis;
    string dbName = "detection-records.sqlite";
    QString GetFormattedDateTime();
};
#endif // MAINWINDOW_H
