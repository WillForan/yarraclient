#include "updatewindow.h"
#include "logging.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile rds(qApp->applicationDirPath() + "/rds.ini");
    if (!rds.exists()) {
        qWarning() << "RDS.ini not found. Is this a valid Yarra Client directory?";
    }

    Logging::init();
    qInfo().noquote() << "Program loaded.";
    class UpdateWindow w;
    w.show();
    int result = a.exec();
    qInfo().noquote() << "Program shutting down.";
    Logging::log_file.close();
    return result;
}
