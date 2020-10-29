#include "updatewindow.h"
#include "logging.h"
#include <QApplication>
#include <iostream>

bool sign(QString path) {
    if (!QFile(path).exists()) {
        std::cout << path.toStdString() << "not found" << std::endl;
        return false;
    }

    QDir dir(path); dir.cdUp();
    QString configPath = dir.filePath("config.ini");

    QSettings config(configPath, QSettings::IniFormat);
    config.setValue("General/Checksum", QString(rdsUpdater::fileChecksum(path).toHex()));

    if (config.value("General/ReleaseNotes").type() == QVariant::Invalid){
        config.setValue("General/ReleaseNotes", "");
    }
    if (config.value("General/ReleaseDate").type() == QVariant::Invalid){
        config.setValue("General/ReleaseDate", QDate::currentDate().toString("yyyy-MM-dd"));
    }
    if (config.value("General/Password").type() == QVariant::Invalid){
        config.setValue("General/Password", "");
    }

    if (config.status() == QSettings::Status::NoError) {
        qInfo().noquote() << "Successfully wrote" << configPath;
        return true;
    } else {
        qDebug() << "Unable to modify config file. Is it a valid INI?" << config.status();
        return false;
    }
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if(argc > 1 && ((strcmp(argv[1],"--help") == 0) ||
            (strcmp(argv[1],"/h") == 0))) {
    qInfo(R"(Yarra Client Update Tool v0.1
  Usage:
      yarra_updater.exe          Display update UI
      yarra_updater.exe --sign   Sign an update package
      yarra_updater.exe --help   Display this usage information)");
    return 0;
    }

    if(argc == 3 && strcmp(argv[1],"--sign") == 0) {
        if (sign(QString::fromLocal8Bit(argv[2]))) {
            return 0;
        } else {
            return 1;
        }
    }

    Logging::init();
    qInfo().noquote() << "Program loaded.";
    class UpdateWindow w;
    w.show();
    QFile rds(qApp->applicationDirPath() + "/rds.ini");
    if (!rds.exists()) {
        qWarning() << "RDS.ini not found. Is the Updater started in a valid Yarra Client directory?";
        QMessageBox::critical(w.centralWidget(),"Warning","RDS.ini not found. Is this a valid Yarra Client directory?");
    }

    int result = a.exec();
    qInfo().noquote() << "Program shutting down.";
    Logging::log_file.close();
    return result;
}
