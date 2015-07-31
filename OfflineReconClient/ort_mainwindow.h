#ifndef ORT_MAINWINDOW_H
#define ORT_MAINWINDOW_H

#include <QDialog>
#include <../Client/rds_log.h>
#include <../Client/rds_raid.h>
#include "ort_configuration.h"
#include "ort_network.h"
#include "ort_modelist.h"


namespace Ui {
class ortMainWindow;
}

class ortMainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ortMainWindow(QWidget *parent = 0);
    ~ortMainWindow();

    // Re-used components from RDS
    rdsLog log;
    rdsRaid raid;
    // Dummy configuration object, needed for the RTI class
    rdsConfiguration dummyconfig;

    // Own components for ORT
    ortConfiguration config;
    ortNetwork network;
    ortModeList modeList;

    int scansToShow;

    void updateScanList();
    void addScanItem(int mid, QString patientName, QString protocolName, QDateTime scanTime, qint64 scanSize, int fid, int mode);

    void refreshRaidList();

    void showTransferError(QString msg);


private slots:
    void on_cancelButton_clicked();
    void on_sendButton_clicked();
    void on_logoLabel_customContextMenuRequested(const QPoint &pos);

    void showLogfile();

    void on_loadOlderButton_clicked();
    void on_manualAssignButton_clicked();
    void on_scansWidget_itemSelectionChanged();

    void on_refreshButton_clicked();

    void on_priorityButton_clicked(bool checked);

private:
    Ui::ortMainWindow *ui;

    bool isManualAssignment;
    bool isRaidListAvaible;

};

#endif // ORT_MAINWINDOW_H
