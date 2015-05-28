#ifndef SAC_MAINWINDOW_H
#define SAC_MAINWINDOW_H

#include <QMainWindow>
#include "sac_network.h"
#include "../OfflineReconClient/ort_modelist.h"
#include "../Client/rds_log.h"

#include "../OfflineReconClient/ort_returnonfocus.h"

namespace Ui {
class sacMainWindow;
}

class sacMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit sacMainWindow(QWidget *parent = 0);
    ~sacMainWindow();

    bool restartApp;

    sacNetwork network;
    ortModeList modeList;
    rdsLog log;

    bool firstFileDialog;
    int defaultMode;

    bool paramVisible;

    QString filename;

    void checkValues();
    void findDefaultMode();

    int detectMode(QString protocol);

    QString taskID;
    QString scanFilename;
    int paramValue;
    QString patientName;
    QString accNumber;
    QString mode;
    QString modeReadable;
    QString notification;
    QString taskFilename;
    QString lockFilename;
    QString protocolName;
    QDateTime taskCreationTime;
    qint64 scanFileSize;

    bool generateTaskFile();

    void analyzeDatFile(QString filename, QString& detectedPatname, QString& detectedProtocol);

private slots:
    void on_selectFileButton_clicked();
    void on_cancelButton_clicked();
    void on_sendButton_clicked();
    void on_modeCombobox_currentIndexChanged(int index);
    void on_taskIDEdit_editingFinished();
    void on_logoLabel_customContextMenuRequested(const QPoint &pos);
    void showLogfile();
    void showConfiguration();
    void showFirstConfiguration();

private:
    Ui::sacMainWindow *ui;

    ortReturnOnFocus returnFocusHelper;

};

#endif // SAC_MAINWINDOW_H
