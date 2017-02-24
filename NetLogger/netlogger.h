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
    bool postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status);
    bool postEvent(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info=QString(""), QString data=QString(""));
    ~NetLogger();
protected:
    QString serverPath;
    QString source_id;
    EventInfo::SourceType source_type;
    QNetworkAccessManager* networkManager;
};

#endif // NETLOGGER_H
