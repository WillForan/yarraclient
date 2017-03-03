#ifndef NETLOGGER_H
#define NETLOGGER_H

#include <QString>
#include <QUrlQuery>
#include <QtNetwork>
#include "netlog_events.h"

class NetLogger
{
public:
    NetLogger(QString path, EventInfo::SourceType source_type, QString source_id);
    QNetworkReply* postDataAsync(QUrlQuery query, QString endpt);

    QUrlQuery buildEventQuery(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data);
    bool postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status);
    void postEvent(EventInfo::Type type, EventInfo::Detail detail =EventInfo::Detail::Generic, EventInfo::Severity severity=EventInfo::Severity::Routine, QString info=QString(""), QString data=QString(""));
    bool postEventSync(EventInfo::Type type, QNetworkReply::NetworkError& error, int& status_code, EventInfo::Detail detail =EventInfo::Detail::Generic, EventInfo::Severity severity=EventInfo::Severity::Routine, QString info=QString(""), QString data=QString(""));

    ~NetLogger();
protected:
    QString serverPath;
    QString source_id;
    EventInfo::SourceType source_type;
    QNetworkAccessManager* networkManager;
};

#endif // NETLOGGER_H
