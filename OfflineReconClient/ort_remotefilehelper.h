#ifndef ORT_REMOTEFILEHELPER_H
#define ORT_REMOTEFILEHELPER_H

#include <QObject>
#include <QtCore>
#include "../Client/rds_exechelper.h"


enum ortConnectionType
{
    UNKNOWN = 0,
    FTP     = 1,
    SFTP    = 2,
    SCP     = 3
};


class ortRemoteFileHelper: public QObject
{
    Q_OBJECT
    rdsExecHelper exec;
    QString serverURI;
    QString winSCPPath;
    QString hostkey;

public:
    ortConnectionType connectionType;
    QString remoteBasePath;
    explicit ortRemoteFileHelper(QObject *parent = nullptr);
    void init(QString server, QString hostKey="acceptnew");
    bool testConnection(QString& error);
    bool callWinSCP(QStringList str, QStringList &output);
    bool runServerOperation(QString operation, QStringList &output);
    bool runServerOperations(QStringList str, QStringList &output);
    qlonglong size(QString path);
    bool exists(QString path);
    bool exists(QStringList path);

    bool get(QString path, QDir dest);
    bool get(QStringList path, QDir dest);
    bool put(QString path);
    bool read(QString path, QString& output);

signals:

};

#endif // ORT_REMOTEFILEHELPER_H
