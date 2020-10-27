#ifndef RDS_UPDATER_H
#define RDS_UPDATER_H

#include <QDialog>
#include <QtWidgets>
#include <logging.h>
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
    bool doVersionUpdate(QString updateVersion, QString &error, QProgressBar* progress);
    versionDetails getVersionDetails(QString version);

    static QByteArray fileChecksum(const QString &fileName,
                            QCryptographicHash::Algorithm hashAlgorithm=QCryptographicHash::Md5);
    static bool copyPath(QString src, QString dst, QString &error, bool overwrite=false, QSet<QString> skip={});
    static QString rot13( const QString & input );
    static bool isValidPackage(QString updateFolder, QString &error);
    static bool copyFileOverwrite(QFile& src, QString dest, QString& error);
    static bool removeFileIfExists(QFile& file,QString& error);
    static bool removeDirIfExists(QDir& dir);
    static bool isRunning(const QString &process);
    static bool mergeSettings(QString path,QDir& fromDir,QString& error);
    static bool mergeSettings(QStringList paths,QDir& fromDir,QString& error);
    static bool extractZip(QString path, QDir& tempFilesDir, QProgressBar* progress, QString& error);
public:
    QString updateFolder;
};

#endif // RDS_UPDATER_H
