#include "rds_updater.h"

#include <zip_file.h>
#include <logging.h>

versionDetails rdsUpdater::getVersionDetails(QString version) {
//    if (!RTI_CONFIG) {
//        return { QDate(), "",""};
//    };
    QString versionFolder = updateFolder+"/"+version;
    QSettings updateInfo(versionFolder+"/config.ini",QSettings::IniFormat);
    QString releaseDate  = updateInfo.value("General/ReleaseDate", "1970-01-01").toString();
    QString releaseNotes = updateInfo.value("General/ReleaseNotes", "").toString();

    QDate releaseQDate = QLocale("en_US").toDate(releaseDate.simplified(), "yyyy-MM-dd");
    QString password = updateInfo.value("General/Password", "").toString();
    bool is_zip = QFile(versionFolder+"/files.zip").exists();
    QString checksum = "";
    if (is_zip)
        checksum = updateInfo.value("General/Checksum", "").toString();
    return {
        releaseQDate, releaseNotes, password, is_zip, checksum
    };
}

bool rdsUpdater::isValidPackage(QString updateFolder, QString &error) {
    QString updateFiles = updateFolder+"/files";
    bool configFileExists = QFile(updateFolder+"/config.ini").exists();
    bool updateFilesExists = QDir(updateFiles).exists() || QFile(updateFolder+"/files.zip").exists();
    bool rdsExeExists = QFile(updateFiles+"/RDS.exe").exists() || QFile(updateFolder+"/files.zip").exists();

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


#define FAIL(err) { error=QString(err); \
        qCritical().noquote() << error; \
        return false; }
#define FAIL_WITH(err,...) FAIL(QString(err).arg(__VA_ARGS__))

#define FAIL_IF(k,err) { if (k) FAIL(err) }
#define TRY(k,err) FAIL_IF(!(k), err)

#define FAIL_IF_WITH(k,err,...) { if (k) { FAIL_WITH(err,__VA_ARGS__)} }

#define TRY_WITH(k,err,...) FAIL_IF_WITH(!(k), err,__VA_ARGS__)

bool rdsUpdater::removeFileIfExists(QFile& file,QString& error) {
    if (file.exists()){
        TRY(file.remove(), file.errorString())
    }
    return true;
}

bool rdsUpdater::copyFileOverwrite(QFile& srcFile, QString dest, QString& error) {
    QFile destFile(dest);
    TRY_WITH(removeFileIfExists(destFile,error),
             "Unable to remove %1: %2",destFile.fileName(),error);

    TRY_WITH(srcFile.copy(dest),
             "Unable to copy %1: %2", srcFile.fileName(), srcFile.errorString())
    return true;
}

bool rdsUpdater::removeDirIfExists(QDir& dir) {
    if (dir.exists()){
        if( !dir.removeRecursively())
            return false;
    }
    return true;
}

bool rdsUpdater::isRunning(const QString &process) {
  QProcess tasklist;
  tasklist.start(
        "tasklist",
        QStringList() << "/NH"
                      << "/FO" << "CSV"
                      << "/FI" << QString("IMAGENAME eq %1").arg(process));
  tasklist.waitForFinished();
  QString output = tasklist.readAllStandardOutput();
  return output.startsWith(QString("\"%1").arg(process));
}

QDebug operator<<(QDebug out, const std::string& str)
{
    out << QString::fromStdString(str);
    return out;
}
bool rdsUpdater::doVersionUpdate(QString updateVersion, QString& error, QProgressBar* progress) {
    QString versionFolder = updateFolder+"/"+updateVersion;
    QString updateFiles = versionFolder+"/files";

    QDir appDir(qApp->applicationDirPath());
    QDir tempDir(appDir.absolutePath()+"/temp");
    QDir tempFilesDir(tempDir.absolutePath()+"/files");

    qDebug() << tempFilesDir.absolutePath();

    QFile configFile(appDir.filePath("rds.ini"));

    FAIL_IF(isRunning("RDS.exe"), "RDS.exe is still running. Shut down the Yarra Client before continuing.")

    if (!isValidPackage(versionFolder, error)) {
        return false;
    }
    versionDetails packageDetails = getVersionDetails(updateVersion);
    QString password = packageDetails.password;

    // If there's a password set, validate it.
    if (password.size()) {
        QInputDialog pwdDialog;
        pwdDialog.setInputMode(QInputDialog::TextInput);
//        pwdDialog.setWindowIcon(RDS_ICON);
        pwdDialog.setWindowTitle("Password");
        pwdDialog.setLabelText("Update Password:");
        pwdDialog.setTextEchoMode(QLineEdit::Password);

        Qt::WindowFlags flags = pwdDialog.windowFlags();
        flags |= Qt::MSWindowsFixedSizeDialogHint;
        flags &= ~Qt::WindowContextHelpButtonHint;
        pwdDialog.setWindowFlags(flags);

        if (pwdDialog.exec()==QDialog::Rejected)
            return false;

        TRY(rot13(pwdDialog.textValue()).toLower() == password.toLower(),
            "Invalid password. Do you have capslock turned on?"
        )
    }

    // Confirm that you really want to update.
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Update confirmation");
        msgBox.setText(QString("Really update to version <b>%1</b>?").arg(updateVersion));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (msgBox.exec() != QMessageBox::Yes) {
            return false;
        }
    }
    qInfo() << "Updating to version" << updateVersion;

    progress->setVisible(true);
    // Reset the temp directory
    {
        TRY( removeDirIfExists(tempDir),
            "Unable to clear temp directory.")
        TRY( appDir.mkdir("temp"),
            "Create temp directory failed. It may be open in another program.");
        TRY( tempDir.mkdir("files"),
            "Create files directory failed. It may be open in another program.");
    }
    progress->setValue(10);

    if (packageDetails.isZip) {
        QFile zip(versionFolder+"/files.zip");
        TRY_WITH( zip.copy(tempDir.filePath("files.zip")),
            "Unable to download package: %1", zip.errorString());

        QString checksum = fileChecksum(tempDir.filePath("files.zip")).toHex();
        TRY(checksum.size() > 0, "Package integrity error: unable to calculate checksum.")

        TRY_WITH(packageDetails.checksum == checksum,
            "Package integrity error: checksum mismatch, got %1",checksum);

        progress->setValue(30);

        try {
            miniz_cpp::zip_file zip_file(tempDir.filePath("files.zip").toStdString());
            std::pair<bool, std::string> zip_test = zip_file.testzip();
            TRY_WITH(zip_test.first,
                "Package integrity error: failed on file %1",  QString::fromStdString(zip_test.second));
            progress->setValue(60);
            for( auto const & info: zip_file.infolist() ){
                if (info.filename.back() == '/') {
                    TRY(tempFilesDir.mkpath(QString::fromStdString(info.filename)),
                        "Error extracting package.");
                }
                qApp->processEvents();
            }
            zip_file.extractall(tempFilesDir.absolutePath().toStdString());
            progress->setValue(90);
            for( auto const & info: zip_file.infolist() ){
                QString filename = QString::fromStdString(info.filename);
                if (info.filename.back() == '/') {
                    TRY_WITH(QDir(tempFilesDir.filePath(filename)).exists(),
                        "Error extracting package: directory failed to extract: %1", filename );
                } else {
                    QFile file(tempFilesDir.filePath(filename));
                    TRY_WITH(file.exists(),
                        "Error extracting package: file failed to extract: %1", filename);
                    FAIL_IF_WITH(file.size() != qint64(info.file_size),
                        "Error extracting package: file size did not match: %1", filename);
                }
            }
            progress->setValue(95);
        } catch (std::runtime_error e) {
            FAIL_WITH("Invalid zip: %1",e.what());
        }
//            error = QString("Invalid zip: %1").arg(e.what());
//            return false;

        QFile(tempDir.filePath("files.zip")).remove();
    } else {
        TRY_WITH(copyPath(updateFiles, tempDir.absolutePath(), error),
            "Package download failed: %1",error)
        progress->setValue(90);
    }
    // Do a little rename dance with the executable
    QString this_exe_name = QFileInfo(QCoreApplication::applicationFilePath()).fileName();

    if (QFile(tempFilesDir.filePath(this_exe_name)).exists()){
        QFile old_file(appDir.filePath(this_exe_name+".old"));
        TRY_WITH(removeFileIfExists(old_file, error),
            "Unable to remove %1.old: %2",this_exe_name,error)

        QFile this_exe(appDir.filePath(this_exe_name));
        this_exe.rename(appDir.absolutePath()+"/"+this_exe_name+".old");

        QFile this_exe_check(appDir.filePath(this_exe_name));
        for (int i=0; i<5;i++){
            if (this_exe_check.exists()) {
                this_exe.rename(appDir.absolutePath() + "/"+this_exe_name+".old");
            } else {
                break;
            }
        }
        FAIL_IF(this_exe_check.exists(),
            "Failed to remove old executable.")
    }

    // Perform the update...
    TRY_WITH(mergeSettings({"rds.ini","ort.ini"} ,tempFilesDir,error),"Failed to update config file: %1", error);

    if (! copyPath(tempFilesDir.absolutePath(), appDir.absolutePath(), error, true, {"rds.ini","ort.ini"}) ) {
        tempDir.removeRecursively();
        FAIL_WITH("Package overwrite failed: %1",error);
    }
    progress->setValue(100);
    tempDir.removeRecursively();
    qInfo() << "Update complete.";
    return true;
}

bool rdsUpdater::mergeSettings(QStringList paths,QDir& fromDir,QString& error){
    QDir appDir(qApp->applicationDirPath());
    for (auto const& i: paths){
        TRY(mergeSettings(appDir.filePath(i),fromDir,error),error);
    }
    return true;
}
bool rdsUpdater::mergeSettings(QString path,QDir& fromDir,QString& error){
    QFile file(path);
    QFileInfo file_info(file);
    if (!file.exists()) {
        QFile new_file(fromDir.absolutePath()+"/"+file_info.fileName());
        TRY_WITH(new_file.copy(path), "Unable to copy new config file %1",file_info.fileName());
        return true;
    }
    TRY_WITH(rdsUpdater::copyFileOverwrite(file, file.fileName()+".bak", error),
        "Unable to backup to %1.bak. Is it open in another program? %2", file.fileName(),error);

    QSettings currentConfig(QFileInfo(file).absoluteFilePath(),QSettings::IniFormat);
    QSettings newConfig(fromDir.absolutePath()+"/"+file_info.fileName(),QSettings::IniFormat);

    if (currentConfig.status() != QSettings::NoError || newConfig.status() != QSettings::NoError ){
        FAIL_WITH("Error parsing %1",file_info.fileName());
    }
    auto newKeys = newConfig.allKeys();
    auto oldKeys = currentConfig.allKeys();

    auto keysToSet = // newConfig.allKeys().toSet().subtract(oldConfig.allKeys().toSet());
            QSet<QString>(newKeys.begin(), newKeys.end()).subtract(
                QSet<QString>(oldKeys.begin(), oldKeys.end()));
    foreach (const QString &key, keysToSet) {
        currentConfig.setValue(key, newConfig.value(key));
    }
    currentConfig.sync();
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

bool rdsUpdater::copyPath(QString src, QString dst, QString& error, bool overwrite, QSet<QString> skip)
{
    QDir dir(src);
    bool success;
    QString sep = "/";//QDir::separator();
    if (! dir.exists())
        return false;

    foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString dst_path = dst + sep + d;
        TRY_WITH(dir.mkpath(dst_path),
                 "Unable to create directory %1",dst_path);

        success = copyPath(src + sep + d, dst_path, error, overwrite);
        if (! success )
            return false;
    }

    foreach (QString f, dir.entryList(QDir::Files)) {
        QString srcPath = src + sep + f;
        QString dstPath = dst + sep + f;

        QFile srcF(srcPath);

        if (skip.contains(f)){
            continue;
        }

        if (overwrite) {
            success = copyFileOverwrite(srcF,dstPath, error);
            if (! success ) {
                return false;
            }
        } else {
            TRY_WITH(srcF.copy(dstPath),
                             "Failed to copy file %1: %2",srcPath,srcF.errorString())
        }

        QByteArray srcCheck = fileChecksum(srcPath,QCryptographicHash::Md5);
        QByteArray dstCheck = fileChecksum(dstPath,QCryptographicHash::Md5);

        success = !(srcCheck.isEmpty() || dstCheck.isEmpty());
        TRY_WITH(success,
            "Checksum calculation failed for file: %1",srcPath)

        success = srcCheck == dstCheck;
        TRY_WITH(success,
                     "Verification error for file: %1",srcPath);
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
    updateFolder = qApp->applicationDirPath()+"/test_upload/update";
}
