#include "netlogger.h"


NetLogger::NetLogger()
{
    configured=false;
    QString serverPath="";
    QString source_id="";
    source_type=EventInfo::SourceType::Generic;

    // TODO: Read certificate from external file
    QFile certificateFile("certificate/logserver.crt");

    // Read the certificate if possible and then add it
    if (certificateFile.open(QIODevice::ReadOnly))
    {
        const QByteArray certificateContent=certificateFile.readAll();

        // Create a certificate object
        const QSslCertificate certificate(certificateContent);

        // Add this certificate to all SSL connections
        QSslSocket::addDefaultCaCertificate(certificate);
    }

    networkManager = new QNetworkAccessManager();
}


NetLogger::~NetLogger()
{
    if (networkManager!=0)
    {
        delete networkManager;
        networkManager=0;
    }
}


void NetLogger::configure(QString path, EventInfo::SourceType sourceType, QString sourceId,QString APIKey)
{
    serverPath=path;
    source_id=sourceId;
    source_type=sourceType;
    api_key =APIKey;

    if (!serverPath.isEmpty())
    {
        // TODO: Check if local host and server path are on the same domain

        configured=true;
    }
}


QUrlQuery NetLogger::buildEventQuery(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data)
{
    QUrlQuery query;

    // ip and time are filled in on the server
    query.addQueryItem("ip",            "0");
    query.addQueryItem("time",          "0");
    query.addQueryItem("info",          info);
    query.addQueryItem("data",          data);
    query.addQueryItem("type",          QString::number((int)type));
    query.addQueryItem("detail",        QString::number((int)detail));
    query.addQueryItem("severity",      QString::number((int)severity));
    query.addQueryItem("source_id",     source_id);
    query.addQueryItem("source_type",   QString::number((int)source_type));

    return query;
}


void NetLogger::postEvent(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data)
{
    if (!configured)
    {
        return;
    }

    QUrlQuery query=buildEventQuery(type,detail,severity,info,data);
    postDataAsync(query,"Events");
}


bool NetLogger::postEventSync(EventInfo::Type type, QNetworkReply::NetworkError& error, int& status_code, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data)
{
    if (!configured)
    {
        return false;
    }

    QUrlQuery query=buildEventQuery(type,detail,severity,info,data);
    return postData(query,"Events",error,status_code);
}


QNetworkReply* NetLogger::postDataAsync(QUrlQuery query, QString endpt)
{
    if (serverPath.isEmpty())
    {
        return NULL;
    }

    query.addQueryItem("api_key",api_key);
    QUrl serviceUrl = QUrl(serverPath + "/" + endpt);
    serviceUrl.setScheme("https");

    QNetworkRequest req(serviceUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QUrl params;
    params.setQuery(query);
    QByteArray postData = params.toEncoded(QUrl::RemoveFragment);
    postData = postData.remove(0,1); // pop off extraneous "?"

    return networkManager->post(req,postData);
}


// This posts some data by urlencoding is, so that's why the parameter is a URLQuery.
// It returns true if and only if it recieves an HTTP 200 OK response. Otherwise, there's either a network error
// or, if the network succeeded but the server failed, an HTTP status code.

bool NetLogger::postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status)
{
    if (!configured)
    {
        return false;
    }

    QNetworkReply* reply=postDataAsync(query,endpt);

    if (!reply)
    {
        return false;
    }

    // Use eventloop to wait until post event has finished
    // TODO: Is there a timeout mechanism?
    QEventLoop eventLoop;
    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    QTimer::singleShot(10000, &eventLoop, SLOT(quit()));
    if (reply->isRunning())
    {
        eventLoop.exec();
    }

    if (reply->error() != QNetworkReply::NoError)
    {
        error = reply->error();
        return false;
    }
    else
    {
        // Make sure the HTTP status is 200
        http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (http_status != 200)
        {
            return false;
        }
    }

    return true;
}

