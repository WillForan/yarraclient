#include "netlogger.h"

NetLogger::NetLogger(QString path, EventInfo::SourceType source_type, QString source_id)
    : serverPath(path),
      source_id(source_id),
      source_type(source_type)
{

    QFile file(":/certificate/localhost.crt");
    file.open(QIODevice::ReadOnly);
    const QByteArray bytes = file.readAll();

        // Create a certificate object
    const QSslCertificate certificate(bytes);

        // Add this certificate to all SSL connections
    QSslSocket::addDefaultCaCertificate(certificate);

    networkManager = new QNetworkAccessManager();
}

NetLogger::~NetLogger(){
    delete networkManager;
}

QUrlQuery NetLogger::buildEventQuery(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data){
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

void NetLogger::postEvent(EventInfo::Type type, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data){
    QUrlQuery query = buildEventQuery(type,detail,severity,info,data);
    postDataAsync(query,"Events");
}

bool NetLogger::postEventSync(EventInfo::Type type, QNetworkReply::NetworkError& error, int& status_code, EventInfo::Detail detail, EventInfo::Severity severity, QString info, QString data){
    QUrlQuery query = buildEventQuery(type,detail,severity,info,data);
    return postData(query,"Events",error, status_code);
}


QNetworkReply* NetLogger::postDataAsync(QUrlQuery query, QString endpt) {
    QUrl serviceUrl = QUrl(serverPath + "/" + endpt);
    serviceUrl.setScheme("https");

    QNetworkRequest req(serviceUrl);
    QUrl params;
    params.setQuery(query);
    QByteArray postData = params.toEncoded(QUrl::RemoveFragment);
    postData = postData.remove(0,1); // pop off extraneous "?"
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    return networkManager->post(req,postData);
}

// This posts some data by urlencoding is, so that's why the parameter is a URLQuery.
// It returns true if and only if it recieves an HTTP 200 OK response. Otherwise, there's either a network error
// or, if the network succeeded but the server failed, an HTTP status code.
bool NetLogger::postData(QUrlQuery query, QString endpt, QNetworkReply::NetworkError& error, int &http_status) {
    QNetworkReply* reply = postDataAsync(query,endpt);
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
