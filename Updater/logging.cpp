#include "logging.h"
#include <QTextStream>
#include <QDateTime>
#include <iostream>
// Macro hack to send to both the logfile and stdout at the same time
#define WRITE_STREAMS(k) log_stream << k; std::cout << k;

QFile Logging::log_file = {};
NetLogger* Logging::net_logger = nullptr;
bool Logging::net_enabled = false;

// Operator to send QStrings to stdout
std::basic_ostream<char>& operator<<(std::basic_ostream<char>& out, const QString& str)
{
    out << str.toStdString();
    return out;
}

void Logging::loggingOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    QTextStream log_stream(&log_file);

    if (log_file.openMode() == QIODevice::NotOpen)
        log_file.open(QIODevice::Append | QIODevice::Text);

    std::map<QtMsgType,QString> types{{QtDebugMsg,      "Debug"},
                                      {QtInfoMsg,       "Info"},
                                      {QtWarningMsg,    "Warning"},
                                      {QtCriticalMsg,   "Critical"},
                                      {QtFatalMsg,      "Fatal"}};

    log_stream << QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss") << " -- ";
    WRITE_STREAMS(types[type] << ": " << msg);
    QString debugDetails = "";

    if (type != QtInfoMsg && strlen(file) > 0) {
       debugDetails = QString(" (%1: %2, %3)").arg(QString(file), QString::number(context.line), QString(function));
       WRITE_STREAMS(debugDetails);
    }
    WRITE_STREAMS("\n")

    log_stream.flush();

    std::map<QtMsgType,std::pair<EventInfo::Detail,EventInfo::Severity>> types_event{
                                      {QtDebugMsg,      {EventInfo::Detail::Diagnostics, EventInfo::Severity::Success}},
                                      {QtInfoMsg,       {EventInfo::Detail::Information, EventInfo::Severity::Success}},
                                      {QtWarningMsg,    {EventInfo::Detail::Information, EventInfo::Severity::Warning}},
                                      {QtCriticalMsg,   {EventInfo::Detail::Information, EventInfo::Severity::Error}},
                                      {QtFatalMsg,      {EventInfo::Detail::Information, EventInfo::Severity::FatalError}}
    };

    Logging::log(EventInfo::Type::Generic, \
                    types_event[type].first,\
                    types_event[type].second, msg,debugDetails);

    std::cout << std::flush;
}

void Logging::initNetLog() {
//    QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
//    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
//    QSslConfiguration::setDefaultConfiguration(sslConf);
    net_enabled = false;
    net_logger = new NetLogger();

    QSettings settings(qApp->applicationDirPath() + "/rds.ini", QSettings::IniFormat);
    QString name = "";
    if (settings.status() == QSettings::Status::NoError) {
        name = settings.value("General/Name","").toString();
    } else {
        qWarning() << "Failed to load RDS.ini:" << settings.status();
        return;
    }
    if (name.isEmpty()){
        QString infoSerialNumber=QProcessEnvironment::systemEnvironment().value("SERIAL_NUMBER", "0");
        QString infoScannerType =QProcessEnvironment::systemEnvironment().value("PRODUCT_NAME",  "Unknown");
        name = infoScannerType + infoSerialNumber;
    }
    QString logServerPath         =settings.value("LogServer/ServerPath",          "").toString();
    QString logApiKey             =settings.value("LogServer/ApiKey",              "").toString();
    if (logServerPath.isEmpty() or logApiKey.isEmpty()) {
        qWarning() << "Logserver not configured.";
        return;
    }
    net_logger->configure(logServerPath,EventInfo::SourceType::Generic,name,logApiKey,true);

    QNetworkReply::NetworkError net_error;
    int http_status=0;
    QString errorString="n/a";

    QUrlQuery data{{"api_key",logApiKey}};
    bool success=net_logger->postData(data, "test", net_error, http_status, errorString);
    if (!success) {
        qWarning() << "Unable to connect to logserver:" << net_error << "(" << errorString <<") HTTP" << http_status;
        return;
    }

    success = net_logger->postEventSync(net_error, http_status,EventInfo::Type::Boot,EventInfo::Detail::Information);
    if (!success) {
        qWarning() << "Unable to send event to logserver: " << net_error << " (" << errorString <<") HTTP " << http_status;
        return;
    }
    qInfo() << "Connected to logserver at" << logServerPath;
    net_enabled = true;
}

void Logging::log(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data) {
    if (net_enabled)
        net_logger->postEventSync(type, detail, severity, info, data);
}
void Logging::log(QString info, QString data) {
    if (net_enabled)
        net_logger->postEventSync(EventInfo::Type::Generic, \
                              EventInfo::Detail::Information,\
                              EventInfo::Severity::Success, info, data);
}

void Logging::init(){
    log_file.setFileName(qApp->applicationDirPath()+"/update.log");
    qInstallMessageHandler(loggingOutput);
    initNetLog();
}
Logging::Logging()
{
}

;
