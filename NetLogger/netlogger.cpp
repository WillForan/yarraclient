#include "netlogger.h"

NetLogger::NetLogger(QString path, EventInfo::SourceType source_type, QString source_id)
    : serverPath(path),
      source_id(source_id),
      source_type(source_type)
{
    networkManager = new QNetworkAccessManager();
}

NetLogger::~NetLogger(){
    delete networkManager;
}

// post an event to the
bool NetLogger::postEvent(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data){
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

    QNetworkReply::NetworkError error;
    int http_status=0;

    return postData(query,"Events",error,http_status);
}

// This posts some data by urlencoding is, so that's why the parameter is a URLQuery.
// It returns true if and only if it recieves an HTTP 200 OK response. Otherwise, there's either a network error
// or, if the network succeeded but the server failed, an HTTP status code.
bool NetLogger::postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status) {
    QUrl serviceUrl = QUrl(serverPath + "/" + endpt);
    QNetworkRequest req(serviceUrl);
    QUrl params;
    params.setQuery(query);
    QByteArray postData = params.toEncoded(QUrl::RemoveFragment);
    postData = postData.remove(0,1); // pop off extraneous "?"

    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QNetworkReply* reply = networkManager->post(req,postData);

    // Wait until it's finished
    QEventLoop eventLoop;
    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();


    if (reply->error() != QNetworkReply::NoError)
    {
        error = reply->error();
        return false;
    }
    else
    {
        // make sure the HTTP status is 200
        http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (http_status != 200) {
            return false;
        }
    }
    return true;
}
