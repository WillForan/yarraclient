#ifndef ORT_NETWORK_H
#define ORT_NETWORK_H

#include <QtCore>

#include <ort_serverlist.h>
#include <../NetLogger/netlogger.h>

class ortConfiguration;


class ortNetwork : public QObject
{
    Q_OBJECT
public:
    ortNetwork();

    ortConfiguration* configInstance;
    void setConfigInstance(ortConfiguration* instance);

    virtual bool prepare();

    virtual bool openConnection(bool fallback=false);
    virtual void closeConnection();
    virtual bool transferQueueFiles();

    virtual bool reconnectToMatchingServer(QString requiredServerType);

    QString connectCmd;
    QString serverPath;
    QString disconnectCmd;
    QString fallbackConnectCmd;
    int     connectTimeout;

    QString appPath;

    QDir queueDir;
    QDir serverDir;
    QDir serverTaskDir;

    ortServerList serverList;
    QString       selectedServer;
    QString       currentServer;

    QString errorReason;

    QStringList fileList;

    QString currentFilename;
    qint64  currentFilesize;

    virtual void releaseFile();
    virtual bool removeFile();
    virtual bool verifyTransfer();
    virtual bool getFileToProcess(int index);
    virtual bool copyFile();
    virtual bool doReconnectServerEntry(ortServerEntry *selectedEntry);
    virtual QSettings* readModelist(QString& error);

    void cleanLocalQueueDir();

    NetLogger netLogger;
};


#endif // ORT_NETWORK_H
