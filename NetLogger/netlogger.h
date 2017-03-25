#ifndef NETLOGGER_H
#define NETLOGGER_H

#include <QString>
#include <QUrlQuery>
#include <QtNetwork>
#include "netlog_events.h"

class NetLogger
{
public:

    NetLogger();
    ~NetLogger();

    void configure(QString path, EventInfo::SourceType sourceType, QString sourceId);
    bool isConfigured();

    QNetworkReply* postDataAsync(QUrlQuery query, QString endpt);
    QUrlQuery buildEventQuery(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data);

    bool postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status);
    void postEvent(EventInfo::Type type, EventInfo::Detail detail=EventInfo::Detail::Information, EventInfo::Severity severity=EventInfo::Severity::Success, QString info=QString(""), QString data=QString(""));
    bool postEventSync(EventInfo::Type type, QNetworkReply::NetworkError& error, int& status_code, EventInfo::Detail detail =EventInfo::Detail::Information, EventInfo::Severity severity=EventInfo::Severity::Success, QString info=QString(""), QString data=QString(""));

protected:

    bool configured;

    QString serverPath;
    QString source_id;
    EventInfo::SourceType source_type;

    QNetworkAccessManager* networkManager;

};


inline bool NetLogger::isConfigured()
{
    return configured;
}


#endif // NETLOGGER_H
