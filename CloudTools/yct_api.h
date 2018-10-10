#ifndef YCTAPI_H
#define YCTAPI_H

#include <QtWidgets>
#include <QString>
#include <QStringList>


class yctConfiguration;
class ortModeList;
class ycaTask;


class yctTransferInformation
{
public:
    yctTransferInformation();

    QString username;
    QString inBucket;
    QString outBucket;
    QString region;
};


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

    bool    uploadCase  (ycaTask* task, yctTransferInformation* setup, QMutex* mutex=0);
    bool    downloadCase(ycaTask* task, yctTransferInformation* setup, QMutex* mutex=0);

    bool    getJobStatus(QList<ycaTask*>* taskList);

    QString errorReason;

protected:

    yctConfiguration* config;

    QStringList helperAppOutput;
    int callHelperApp(QString cmd);

};


#endif // YCTAPI_H
