#ifndef YCA_MAINWINDOW_H
#define YCA_MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QMenu>

#include "../CloudTools/yct_configuration.h"
#include "yca_transferindicator.h"


namespace Ui
{
    class ycaMainWindow;
}


class ycaMainWindow;

class ycaWorker : public QObject
{
    Q_OBJECT

public:
    ycaWorker();
    void setParent(ycaMainWindow* myParent);

    void shutdown();
    void trigger();

public slots:
    void startTimer();
    void stopTimer();
    void timerCall();

private:
    QThread transferThread;
    QTimer  transferTimer;

    bool processingActive;

    ycaMainWindow* parent;

};


class ycaMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ycaMainWindow(QWidget *parent = 0);
    ~ycaMainWindow();

protected:
    void closeEvent(QCloseEvent* event);

public slots:
    void callShutDown(bool askConfirmation=true);
    void callSubmit();

    void showIndicator();
    void hideIndicator();


private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_closeButton_clicked();
    void on_closeContextButton_clicked();

    void on_statusRefreshButton_clicked();

    void on_pushButton_5_clicked();
    void on_pushButton_3_clicked();

private:
    Ui::ycaMainWindow* ui;

    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    QAction* trayItemShutdown;

    yctConfiguration config;
    ycaTransferIndicator indicator;

    ycaWorker transferWorker;

};

#endif // YCA_MAINWINDOW_H
