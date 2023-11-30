#ifndef REMOTEFILEHELPER_H
#define REMOTEFILEHELPER_H

#include <QObject>
#include <QtCore>
#include "../Client/rds_exechelper.h"

class remoteFileHelper : public QObject
{
    Q_OBJECT
    rdsExecHelper exec;
    QString serverURI;
    QString winSCPPath;
    QString hostkey;
public:
    enum connectionType
    {
        FTP = 0,
        SFTP= 1
    };
    connectionType connectionType;
    explicit remoteFileHelper(QObject *parent = nullptr);
    void init(QString server, enum connectionType connectionType, QString hostKey="acceptnew");
    bool testConnection();
    bool callWinSCP(QStringList str, QStringList &output);
    bool runServerOperation(QString operation, QStringList &output);
    bool runServerOperations(QStringList str, QStringList &output);
    long int size(QString path);
    bool exists(QString path);
    bool get(QString path, QDir dest);
    bool put(QString path);
    bool read(QString path, QString& output);

signals:

};

#endif // REMOTEFILEHELPER_H
