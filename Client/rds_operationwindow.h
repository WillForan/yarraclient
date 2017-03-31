#ifndef RDS_OPERATIONWINDOW_H
#define RDS_OPERATIONWINDOW_H

#include <QDialog>
#include <QtWidgets>
#include <QSystemTrayIcon>

#include "rds_configuration.h"
#include "rds_raid.h"
#include "rds_network.h"
#include "rds_log.h"
#include "rds_processcontrol.h"
#include "rds_debugwindow.h"
#include "rds_iconwindow.h"


namespace Ui
{
    class rdsOperationWindow;
}


class rdsOperationWindow : public QDialog
{
    Q_OBJECT

public:
    explicit rdsOperationWindow(QWidget *parent = 0);
    ~rdsOperationWindow();    

    rdsIconWindow  iconWindow;

public slots:
    void callShutDown();
    void callImmediateShutdown();
    void callConfiguration();
    void callManualUpdate();
    void callShowLogfile();
    void callPostpone();

    void checkForUpdate();

    void setUIModeProgress();
    void setUIModeIdle();

    void updateInfoUI();

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

protected:
    void closeEvent(QCloseEvent *event);    
    void keyPressEvent(QKeyEvent* event);

private:
    Ui::rdsOperationWindow *ui;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    QAction* trayItemTransferNow;
    QAction* trayItemConfiguration;
    QAction* trayItemShutdown;

    rdsConfiguration config;
    rdsRaid raid;
    rdsLog log;
    rdsNetwork network;
    rdsProcessControl control;

    QTimer controlTimer;

    rdsDebugWindow debugWindow;
};


#endif // RDS_OPERATIONWINDOW_H

