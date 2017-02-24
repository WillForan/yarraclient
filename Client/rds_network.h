#ifndef RDS_NETWORK_H
#define RDS_NETWORK_H

#include <QtCore>
#include <QtNetwork>

#include <../NetLogger/netlogger.h>

//#include <QFtp>

class rdsNetwork : public QObject
{
    Q_OBJECT

public:

    rdsNetwork();
    ~rdsNetwork();

    // External interface

    bool openConnection();
    void closeConnection();
    bool isConnectionActive();

    bool checkExistance(QString filename);
    bool transferFiles();


    // Internal methods

    bool isQueueEmpty();
    int getQueueCount();

    bool getFileToProcess(int index);
    bool copyFile();
    bool verifyTransfer();
    bool removeFile();
    void releaseFile();

    void copyLogFile();
    void runReconnectCmd();

    NetLogger *netLogger;
private:

    QDir queueDir;
    bool connectionActive;

    QStringList fileList;

    QString currentFilename;
    QString currentProt;
    qint64  currentFilesize;
    QString currentTimeStamp;

    //QFtp ftp;
    QDir networkDrive;

    // TODO: Implement event handler for FTP management
};


inline bool rdsNetwork::isConnectionActive()
{
    return connectionActive;
}



class rdsCopyThread : public QThread
{
public:
    rdsCopyThread();

    void run();

    QString sourceName;
    QString destName;
    bool success;

    bool finishedCopy;
};


#endif // RDS_NETWORK_H

