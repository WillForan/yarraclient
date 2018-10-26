#ifndef YCTAPI_H
#define YCTAPI_H

#include <QtWidgets>
#include <QString>
#include <QStringList>

#include "yct_common.h"


class yctConfiguration;
class ortModeList;
class ycaTask;


class yctTransferInformation
{
public:
    yctTransferInformation();

    QString username;
    bool    userAllowed;

    QString inBucket;
    QString outBucket;
    QString region;    
};


class yctStorageInformation
{
public:
    yctStorageInformation();

    enum StorageMethod
    {
        ystInvalid=0,
        yctDrive=1,
        yctPACS=2
    };

    QString       name;
    StorageMethod method;

    QString pacsIP;
    QString pacsPort;
    QString pacsAET;
    QString pacsAEC;

    QString driveLocation;

};

typedef QList<yctStorageInformation*> yctStorageList;


class yctAPI : public QObject
{
    Q_OBJECT

public:
    yctAPI();
    void    setConfiguration(yctConfiguration* configuration);

#ifdef YARRA_APP_SAC
    int     readModeList(ortModeList* modeList);
    void    launchCloudAgent(QString params="");
#endif
    bool    createCloudFolders();
    bool    validateUser(yctTransferInformation* transferInformation=0);
    QString getCloudPath(QString folder);
    QString createUUID();

    bool    uploadCase        (ycaTask* task, yctTransferInformation* setup, QMutex* mutex=0);
    bool    downloadCase      (ycaTask* task, yctTransferInformation* setup, QMutex* mutex=0);
    bool    getJobStatus      (QList<ycaTask*>* taskList);
    bool    insertPHI         (QString path, ycaTask* task);

    bool    pushToDestinations(QString path, ycaTask* task);
    bool    pushToPACS (QString path, ycaTask* task, yctStorageInformation* destination);
    bool    pushToDrive(QString path, ycaTask* task, yctStorageInformation* destination);

    QString errorReason;

protected:

    yctConfiguration* config;

    QStringList helperAppOutput;
    int callHelperApp(QString binary, QString parameters, int execTimeout=YCT_HELPER_TIMEOUT);

};


#endif // YCTAPI_H
