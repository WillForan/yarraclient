#ifndef ORT_NETWORK_H
#define ORT_NETWORK_H

#include <QtCore>

class ortNetwork : public QObject
{
    Q_OBJECT
public:
    ortNetwork();

    bool prepare();

    bool openConnection(bool fallback=false);
    void closeConnection();    
    bool transferQueueFiles();

    QString connectCmd;
    QString serverPath;
    QString disconnectCmd;
    QString fallbackConnectCmd;
    int connectTimeout;

    QString appPath;

    QDir queueDir;
    QDir serverDir;
    QDir serverTaskDir;

    QStringList fileList;

    QString currentFilename;
    qint64  currentFilesize;

    void releaseFile();
    bool removeFile();
    bool verifyTransfer();
    bool getFileToProcess(int index);
    bool copyFile();

    void cleanLocalQueueDir();

};

#endif // ORT_NETWORK_H
