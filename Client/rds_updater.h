#ifndef RDS_UPDATER_H
#define RDS_UPDATER_H

#include "rds_configuration.h"
#include "rds_global.h"
#include <QDialog>

struct versionDetails {
    QDate releaseDate;
    QString releaseNotes;
    QString password;
};

class rdsUpdater
{
public:
    rdsUpdater();
    static bool doVersionUpdate(QString updateVersion, QString &error);
    static QByteArray fileChecksum(const QString &fileName,
                            QCryptographicHash::Algorithm hashAlgorithm);
    static bool copyPath(QString src, QString dst, QString &error);
    static versionDetails getVersionDetails(QString version);
    static QString rot13( const QString & input );
    static bool isValidPackage(QString updateFolder, QString &error);
};

#endif // RDS_UPDATER_H
