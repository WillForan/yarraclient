#include "netlogger.h"

#ifdef YARRA_APP_RDS
    #include "rds_global.h"
#endif


NetLogger::NetLogger()
{
    configured=false;
    configurationError=false;

    QString serverPath="";
    QString source_id="";
    source_type=EventInfo::SourceType::Generic;

    // Read certificate from external file
    QFile certificateFile(qApp->applicationDirPath() + "/logserver.crt");

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


bool NetLogger::isServerInSameDomain(QString serverPath)
{
    QString errorMessage="";
    bool error=false;

    int serverPort=8080;

    if (serverPath.isEmpty())
    {
        errorMessage="No server address entered.";
        error=true;
    }

    QString localHostname="";

    if (!error)
    {
        // Check if the entered address contains a port specifier
        int colonPos=serverPath.indexOf(":");

        if (colonPos!=-1)
        {
            int cutChars=serverPath.length()-colonPos;

            QString portString=serverPath.right(cutChars-1);
            serverPath.chop(cutChars);

            // Convert port string into number and validate
            bool portValid=false;

            int tempPort=portString.toInt(&portValid);

            if (portValid)
            {
                serverPort=tempPort;
            }
        }


        // Open socket connection to see if the server is active and to
        // determine the local IP address used for routing to the server.
        QTcpSocket socket;
        socket.connectToHost(serverPath, serverPort);

        if (socket.waitForConnected(500))
        {
            localHostname=QHostInfo::fromName( socket.localAddress().toString() ).hostName();

        }
        else
        {
            errorMessage="Unable to connect to server.";
            error=true;
        }

        socket.disconnectFromHost();
    }

    if ((!error) && (localHostname.isEmpty()))
    {
        errorMessage="Unable to resolve local hostname.";
        error=true;
    }

    // Now compare if the local system and the server live on the same domain
    if (!error)
    {
        serverPath=QHostInfo::fromName(serverPath).hostName();

        QStringList localHostString =localHostname.toLower().split(".");
        QStringList serverHostString=serverPath.toLower().split(".");

        if ((localHostString.count()<2) || (serverHostString.count()<2))
        {
            errorMessage="Error resolving hostnames.";
            error=true;
        }
        else
        {
            if ((localHostString.at(localHostString.count()-1)!=(serverHostString.at(serverHostString.count()-1)))
               || (localHostString.at(localHostString.count()-2)!=(serverHostString.at(serverHostString.count()-2))))
            {
                errorMessage="Log server not in local domain.";
                error=true;
            }
        }
    }

    // Report the error, using an error reporting mechanism depending on
    // what binary target this class is built into
    if (error)
    {
    #ifdef YARRA_APP_RDS
        RTI->log("ERROR: Configuration of log server failed.");
        RTI->log("ERROR: " + errorMessage);
        RTI->log("ERROR: Use configuration dialog to analyze connection.");
        RTI->log("ERROR: Logging has been disabled.");
    #endif

        return false;
    }

#ifdef YARRA_APP_RDS
    RTI->log("Connection to log server validated.");
#endif

    return true;
}


void NetLogger::configure(QString path, EventInfo::SourceType sourceType, QString sourceId)
{
    configurationError=false;
    serverPath=path;
    source_id=sourceId;
    source_type=sourceType;

    if (!serverPath.isEmpty())
    {
        // Check if local host and server path are on the same domain
        configured=isServerInSameDomain(path);

        if (!configured)
        {
            configurationError=true;
        }
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

