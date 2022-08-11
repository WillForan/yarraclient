#ifndef YD_MAINWINDOW_H
#define YD_MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "yd_test.h"


namespace Ui {
class ydMainWindow;
}

class ydMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ydMainWindow(QWidget *parent = 0);
    ~ydMainWindow();

    void composeTests();

private slots:
    void on_runButton_clicked();

    void timerCall();

    void on_exportButton_clicked();

private:
    Ui::ydMainWindow *ui;

    QTimer updateTimer;
    ydTestRunner testRunner;
};

#endif // YD_MAINWINDOW_H
