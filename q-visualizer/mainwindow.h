#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <QtCharts>

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
    void DisplayLineChart(QLineSeries *series);
    int ReadDataFromDB(QLineSeries *series);
    static int OnRowFetched(void *data, int colCount, char** colValues, char** colNames)
    {
        for (int i=0; i<colCount; i++)
        {
            cout << colNames[i] << " = " << (colValues[i] ? colValues[i] : "NULL") << ";";
        }
        cout << endl;
        return 0;
        // If an sqlite3_exec() callback returns non-zero, the sqlite3_exec() routine
        // returns SQLITE_ABORT without invoking the callback again and without running any subsequent SQL statements.
    }

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
