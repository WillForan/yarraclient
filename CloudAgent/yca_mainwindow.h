#ifndef YCA_MAINWINDOW_H
#define YCA_MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QMenu>

#include "../CloudTools/yct_configuration.h"
#include "yca_transferindicator.h"
#include "yca_task.h"
#include "../CloudTools/yct_api.h"


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

    bool                   processingActive;
    bool                   processingPaused;
    ycaTask::WorkerProcess currentProcess;
    QString                currentTaskID;


public slots:
    void startTimer();
    void stopTimer();
    void timerCall();

private:
    QThread transferThread;
    QTimer  transferTimer;

    yctTransferInformation transferInformation;
    bool                   userInvalidShown;

    ycaMainWindow* parent;

    void updateParentStatus();

};


class ycaMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ycaMainWindow(QWidget *parent = 0);
    ~ycaMainWindow();

protected:
    void closeEvent(QCloseEvent* event);
    void showEvent(QShowEvent* event);

public slots:
    void callShutDown(bool askConfirmation=true);
    void callSubmit();

    void showIndicator();
    void hideIndicator();
    void showNotification(QString text);
    void showStatus(QString text);    

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void updateUI();
    void updateUIWorker();

    void on_closeButton_clicked();
    void on_closeContextButton_clicked();
    void on_statusRefreshButton_clicked();
    void on_detailsButton_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_transferButton_clicked();
    void on_pauseButton_clicked();
    void on_clearArchiveButton_clicked();
    void on_notificationsCheckbox_clicked();
    void on_refreshArchiveButton_clicked();
    void on_archiveDetailsButton_clicked();
    void on_searchButton_clicked();
    void on_searchEdit_returnPressed();
    void on_detailsSearchButton_clicked();
    void on_refreshLogButton_clicked();
    void on_externalLogButton_clicked();
    void on_logDetailCombobox_currentIndexChanged(int index);    
    void on_logTicketButton_clicked();
    void on_logClipboardButton_clicked();

    void on_logWidget_customContextMenuRequested(const QPoint &pos);
    void logCalcDuration();

private:
    Ui::ycaMainWindow* ui;
    QSystemTrayIcon*   trayIcon;
    QMenu*             trayIconMenu;
    QAction*           trayItemShutdown;

    ycaTransferIndicator indicator;
    ycaWorker            transferWorker;
    ycaTaskList          taskList;
    ycaTaskList          archiveList;

    bool checkForDCMTK();

public:
    yctConfiguration     config;
    ycaTaskHelper        taskHelper;
    yctAPI               cloud;
    QMutex               mutex;
    bool                 shuttingDown;

};


#endif // YCA_MAINWINDOW_H

