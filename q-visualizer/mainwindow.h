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
    void DisplayLineChart(QLineSeries *series, int minVal, int maxVal);
    int ReadDataFromDB(QLineSeries *series, int &minVal, int &maxVal);
    int GetDBFromSFTP();

private slots:
     void on_pushButtonLoad_clicked();

private:
    Ui::MainWindow *ui;
    QChart *chart = NULL;
    // Should be set to NULL otherwise delete chart may lead to segment fault
    string dbName = "detection-records.sqlite";
    QString GetFormattedDateTime();
};
#endif // MAINWINDOW_H
