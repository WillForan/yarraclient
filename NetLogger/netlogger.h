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

    void configure(QString path, EventInfo::SourceType sourceType, QString sourceId, QString key, bool skipDomainValidation=false);
    bool isConfigured();
    bool isConfigurationError();
    bool isServerInSameDomain(QString serverPath);
    bool retryDomainValidation();

    QNetworkReply* postDataAsync(QUrlQuery query, QString endpt);
    QUrlQuery buildEventQuery(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data);

    bool postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status, QString &errorString);
    void postEvent(EventInfo::Type type, EventInfo::Detail detail=EventInfo::Detail::Information, EventInfo::Severity severity=EventInfo::Severity::Success, QString info=QString(""), QString data=QString(""));
    bool postEventSync(EventInfo::Type type, QNetworkReply::NetworkError& error, int& status_code, EventInfo::Detail detail =EventInfo::Detail::Information, EventInfo::Severity severity=EventInfo::Severity::Success, QString info=QString(""), QString data=QString(""));

    static QString dnsLookup(QString address);

protected:

    bool configured;
    bool configurationError;

    QString serverPath;
    QString apiKey;
    QString source_id;
    EventInfo::SourceType source_type;

    QNetworkAccessManager* networkManager;

};


inline bool NetLogger::isConfigured()
{
    return configured;
}


inline bool NetLogger::isConfigurationError()
{
    return configurationError;
}


#endif // NETLOGGER_H
