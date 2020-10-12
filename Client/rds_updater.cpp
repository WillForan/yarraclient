#include "rds_updater.h"

#include <QtWidgets>

versionDetails rdsUpdater::getVersionDetails(QString version) {
//    if (!RTI_CONFIG) {
//        return { QDate(), "",""};
//    };
    QString updateFolder = RTI_CONFIG->rdsUpdatePath+"/"+version;
    QSettings updateInfo(updateFolder+"/config.ini",QSettings::IniFormat);
    QString releaseDate  = updateInfo.value("General/ReleaseDate", "1970-01-01").toString();
    QString releaseNotes = updateInfo.value("General/ReleaseInfo", "").toString();

    QDate releaseQDate = QLocale("en_US").toDate(releaseDate.simplified(), "yyyy-MM-dd");
    QString password = updateInfo.value("General/Password", "").toString();
    return {
        releaseQDate, releaseNotes, password
    };
}

bool rdsUpdater::isValidPackage(QString updateFolder, QString &error) {
    QString updateFiles = updateFolder+"/files";
    bool configFileExists = QFile(updateFolder+"/config.ini").exists();
    bool updateFilesExists = QDir(updateFiles).exists();
    bool rdsExeExists = QFile(updateFiles+"/RDS.exe").exists();

    if (!(configFileExists && updateFilesExists && rdsExeExists)) {
        QString err = QString("Package is missing required files: ");
        if (!configFileExists) {
            err += "\n./config.ini";
        }
        if (!updateFilesExists)  {
            err += "\n./files";
        }
        if (!updateFilesExists)  {
            err += "\n./RDS.exe";
        }
        error = err;
        return false;
    }
    QSettings updateInfo(updateFolder+"/config.ini",QSettings::IniFormat);

    if (updateInfo.status() != QSettings::NoError) {
        error = "Package configuration missing or corrupted.";
        return false;
    }

    return true;
}

bool rdsUpdater::doVersionUpdate(QString updateVersion, QString& error) {
    QString updateFolder = RTI_CONFIG->rdsUpdatePath+"/"+updateVersion;
    QString updateFiles = updateFolder+"/files";

    QDir tempDir(RTI->getAppPath()+"/temp");

    if (!isValidPackage(updateFolder, error)) {
        return false;
    }

    QString password = getVersionDetails(updateVersion).password;

    // If there's a password set, validate it.
    if (password.size()) {
        QInputDialog pwdDialog;
        pwdDialog.setInputMode(QInputDialog::TextInput);
        pwdDialog.setWindowIcon(RDS_ICON);
        pwdDialog.setWindowTitle("Password");
        pwdDialog.setLabelText("Update Password:");
        pwdDialog.setTextEchoMode(QLineEdit::Password);

        Qt::WindowFlags flags = pwdDialog.windowFlags();
        flags |= Qt::MSWindowsFixedSizeDialogHint;
        flags &= ~Qt::WindowContextHelpButtonHint;
        pwdDialog.setWindowFlags(flags);

        if (pwdDialog.exec()==QDialog::Rejected)
            return false;

        if (rot13(pwdDialog.textValue()) != password) {
            error = "Invalid password. Do you have capslock turned on?";
            return false;
        }
    }

    // Confirm that you really want to update.
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Update confirmation");
        msgBox.setText(QString("Really update from version <b>%1</b> to <b>%2</b>?").arg(RDS_VERSION,updateVersion));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (msgBox.exec() != QMessageBox::Yes) {
            return false;
        }
    }

    // Reset the temp directory
    {
        if (tempDir.exists()) {
            tempDir.removeRecursively();
        }
        QDir appDir(RTI->getAppPath());
        appDir.mkdir("temp");
    }
    if ( ! copyPath(updateFiles, tempDir.absolutePath(), error) ) {
        error = QString("Package download failed: %1").arg(error);
        return false;
    }

    if (!isValidPackage(tempDir.absolutePath(), error)) {
        return false;
    }

    // Do a little rename dance with the RDS executable
    {
        QFile old_file(RTI->getAppPath() + "/RDS.exe.old");
        if (old_file.exists())
            old_file.remove();

        QFile this_exe(RTI->getAppPath() + "/RDS.exe");
        this_exe.rename(RTI->getAppPath() + "/RDS.exe.old");

        QFile this_exe_check(RTI->getAppPath() + "/RDS.exe");
        for (int i=0; i<5;i++){
            if (this_exe_check.exists()) {
                this_exe.rename(RTI->getAppPath() + "/RDS.exe.old");
            } else {
                break;
            }
        }
        if (this_exe_check.exists()) {
            error = "Failed to remove old executable.";
            return false;
        }
    }

    // Perform the update...

    QFile oldConfigFile(RTI->getAppPath()+"/config.ini");
    QFile(RTI->getAppPath()+"/config.ini.bak").remove();
    oldConfigFile.copy(RTI->getAppPath()+"/config.ini.bak");

    QFile newConfigFile(tempDir.absolutePath()+"/config.ini");

    if (! copyPath(tempDir.absolutePath(), RTI->getAppPath(), error) ) {
        tempDir.removeRecursively();
        error = QString("Package overwrite failed: %1").arg(error);
        return false;
    }

    tempDir.removeRecursively();
    return true;
}

QByteArray rdsUpdater::fileChecksum(const QString &fileName,
                        QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&f)) {
            return hash.result();
        }
    }
    return QByteArray();
}

bool rdsUpdater::copyPath(QString src, QString dst, QString& error) // std::function< bool(QString,QString) >& each_file
{
    QDir dir(src);
    bool success;
    QString sep = QDir::separator();
    if (! dir.exists())
        return false;

    foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString dst_path = dst + sep + d;
        success = dir.mkpath(dst_path);
        if (! success ) {
            error = QString("Unable to create directory %1").arg(dst_path);
            return false;
        }
        success = copyPath(src + sep + d, dst_path, error);
        if (! success )
            return false;
    }

    foreach (QString f, dir.entryList(QDir::Files)) {
        QString srcPath = src + sep + f;
        QString dstPath = dst + sep + f;

        QFile srcF(srcPath);

        success = srcF.copy(dstPath);
        if (! success ) {
            error = QString("Failed to copy file %1: %2").arg(srcPath,srcF.errorString());
            return false;
        }
        QByteArray srcCheck = fileChecksum(srcPath,QCryptographicHash::Md5);
        QByteArray dstCheck = fileChecksum(dstPath,QCryptographicHash::Md5);

        success = (srcCheck.size() == 0 || dstCheck.size() == 0);
        if (! success ) {
            error = QString("Checksum calculation failed for file: %1").arg(srcPath);
            return false;
        }
        if (srcCheck != dstCheck) {
            error = QString("Verification error for file: %1").arg(srcPath);
            return false;
        }
    }
    return true;
}

QString rdsUpdater::rot13( const QString & input )
{
    QString r = input;
    int i = r.length();
    while( i-- ) {
        if ( (r[i] >= QChar('A') && r[i] <= QChar('M')) ||
             (r[i] >= QChar('a') && r[i] <= QChar('m')) )
            r[i] = (char)((int)QChar(r[i]).toLatin1() + 13);
        else if  ( (r[i] >= QChar('N') && r[i] <= QChar('Z')) ||
                   (r[i] >= QChar('n') && r[i] <= QChar('z')) )
            r[i] = (char)((int)QChar(r[i]).toLatin1() - 13);
    }
    return r;
}


rdsUpdater::rdsUpdater()
{

}
