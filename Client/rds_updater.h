#ifndef RDS_UPDATER_H
#define RDS_UPDATER_H

#include "rds_configuration.h"
#include "rds_global.h"
#include <QDialog>
#include <QtWidgets>

struct versionDetails {
    QDate releaseDate;
    QString releaseNotes;
    QString password;
    bool isZip;
    QString checksum;
};

class rdsUpdater
{
public:
    rdsUpdater();
    static bool doVersionUpdate(QString updateVersion, QString &error, QProgressBar* progress);
    static QByteArray fileChecksum(const QString &fileName,
                            QCryptographicHash::Algorithm hashAlgorithm=QCryptographicHash::Md5);
    static bool copyPath(QString src, QString dst, QString &error, bool overwrite=false, QSet<QString> skip={});
    static versionDetails getVersionDetails(QString version);
    static QString rot13( const QString & input );
    static bool isValidPackage(QString updateFolder, QString &error);
    static bool copyFileOverwrite(QFile& src, QString dest, QString& error);
    static bool removeFileIfExists(QFile& file,QString& error);
    static bool removeDirIfExists(QDir& dir);
};

#endif // RDS_UPDATER_H
