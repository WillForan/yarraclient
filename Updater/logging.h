#ifndef LOGGING_H
#define LOGGING_H

#include <QApplication>
#include <QFile>
#include "../NetLogger/netlogger.h"
class Logging
{
public:
    Logging();
    static QFile log_file;
    static void loggingOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void init();
    static void log(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data);
    static void log(QString info, QString data="");
private:
    static void initNetLog();
    static NetLogger* net_logger;
    static bool net_enabled;
};

#endif // LOGGING_H
