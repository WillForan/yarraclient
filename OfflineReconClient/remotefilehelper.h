#ifndef REMOTEFILEHELPER_H
#define REMOTEFILEHELPER_H

#include <QObject>
#include <QtCore>
#include "../Client/rds_exechelper.h"

enum connectionType
{
    UNKNOWN = 0,
    FTP= 1,
    SFTP=2,
    SCP=3
};

class remoteFileHelper : public QObject
{
    Q_OBJECT
    rdsExecHelper exec;
    QString serverURI;
    QString winSCPPath;
    QString hostkey;
public:
    connectionType connectionType;
    explicit remoteFileHelper(QObject *parent = nullptr);
    void init(QString server, QString hostKey="acceptnew");
    bool testConnection(QString& error);
    bool callWinSCP(QStringList str, QStringList &output);
    bool runServerOperation(QString operation, QStringList &output);
    bool runServerOperations(QStringList str, QStringList &output);
    long int size(QString path);
    bool exists(QString path);
    bool exists(QStringList path);

    bool get(QString path, QDir dest);
    bool get(QStringList path, QDir dest);
    bool put(QString path);
    bool read(QString path, QString& output);

signals:

};

#endif // REMOTEFILEHELPER_H
