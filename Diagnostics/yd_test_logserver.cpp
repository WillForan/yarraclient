#include "yd_test_logserver.h"
#include "yd_global.h"
#include "yd_helper.h"
#include "../Client/rds_exechelper.h"
#include "../Netlogger/netlogger.h"

#include <QTcpSocket>
#include <QNetworkProxy>
#include <QUrlQuery>
#include <QNetworkReply>


ydTestLogServer::ydTestLogServer() : ydTest()
{
    logServerAddress="";
    logServerAPIKey="";
}


QString ydTestLogServer::getName()
{
    return "LogServer";
}


QString ydTestLogServer::getDescription()
{
    return "indentify LogServer issues";
}


bool ydTestLogServer::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION

    if ((!broker->contains("logserver_address")) || (!broker->contains("logserver_apikey")))
    {
        YD_ADDRESULT("LogServer has not been configured");
        return true;
    }

    logServerAddress=broker->value("logserver_address");
    logServerAPIKey=broker->value("logserver_apikey");

    YD_ADDRESULT("LogServer settings (from "+broker->value("logserver_source")+" configuration):");
    YD_ADDRESULT_LINE(" - Address: " + logServerAddress);
    YD_ADDRESULT_LINE(" - API key: " + logServerAPIKey);
    YD_RESULT_ENDSECTION

    YD_RESULT_STARTSECTION
    YD_ADDRESULT("Checking for certificate file...");
    QDir appDir(qApp->applicationDirPath());
    if (!appDir.exists("logserver.crt"))
    {
        YD_ADDRESULT_COLORLINE("LogServer certificate file is missing", YD_CRITICAL);
        YD_ADDISSUE("LogServer certificate file missing", YD_CRITICAL);
        return true;
    }
    else
    {
        YD_ADDRESULT_COLORLINE("Certificate file found", YD_SUCCESS);
    }
    YD_RESULT_ENDSECTION

    testConnection(issues, results);

    return true;
}


void ydTestLogServer::testConnection(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION
    YD_ADDRESULT("Testing connectivity...");

    QString serverPath=logServerAddress;
    int serverPort=8080;

    // Check if the entered address contains a port specifier
    int colonPos=serverPath.indexOf(":");

    if (colonPos!=-1)
    {
        int cutChars=serverPath.length()-colonPos;

        bool portValid=false;
        QString portString=serverPath.right(cutChars-1);
        serverPath.chop(cutChars);
        int tempPort=portString.toInt(&portValid);
        if (portValid)
        {
            serverPort=tempPort;
        }
    }

    QString localHostname="";
    QString localIP="";

    // Open socket connection to see if the server is active and to
    // determine the local IP address used for routing to the server.
    QTcpSocket socket;
    socket.setProxy(QNetworkProxy::NoProxy);
    socket.connectToHost(serverPath, serverPort);

    if (socket.waitForConnected(10000))
    {
        YD_ADDRESULT_COLORLINE("Able to connect to LogServer", YD_SUCCESS);
        localIP=socket.localAddress().toString();
    }
    else
    {
        YD_ADDRESULT_COLORLINE("Unable to connect to LogServer", YD_CRITICAL);
        YD_ADDISSUE("Unable to connect to LogServer", YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return;
    }

    socket.disconnectFromHost();

    YD_ADDRESULT_LINE("Local IP for connection to LogServer: " + localIP);
    localHostname=NetLogger::dnsLookup(localIP);

    if (localHostname.isEmpty())
    {
        YD_ADDRESULT_COLORLINE("Unable to resolve local hostname", YD_CRITICAL);
        YD_ADDRESULT_COLORLINE("Check local DNS server settings", YD_CRITICAL);
        YD_ADDISSUE("Unable to resolve local hostname", YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return;
    }

    YD_ADDRESULT_LINE("Local hostname: " + localHostname);

    // Lookup the hostname of the log server from the DNS server
    serverPath=NetLogger::dnsLookup(serverPath);

    if (serverPath.isEmpty())
    {
        YD_ADDRESULT_COLORLINE("Unable to resolve LogServer hostname", YD_CRITICAL);
        YD_ADDRESULT_COLORLINE("Check local DNS server settings", YD_CRITICAL);
        YD_ADDISSUE("Unable to resolve LogServer hostname", YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return;
    }

    QStringList localHostString =localHostname.toLower().split(".");
    QStringList serverHostString=serverPath.toLower().split(".");

    if ((localHostString.count()<2) || (serverHostString.count()<2))
    {
        YD_ADDRESULT_COLORLINE("Error resolving hostnames", YD_CRITICAL);
        YD_ADDRESULT_COLORLINE("Localhost: " + localHostname.toLower(), YD_CRITICAL);
        YD_ADDRESULT_COLORLINE("LogServer: " + serverHostString.join("."), YD_CRITICAL);
        YD_ADDISSUE("Error resolving hostnames", YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return;
    }
    else
    {
        if ((localHostString.at(localHostString.count()-1)!=(serverHostString.at(serverHostString.count()-1)))
           || (localHostString.at(localHostString.count()-2)!=(serverHostString.at(serverHostString.count()-2))))
        {
            YD_ADDRESULT_COLORLINE("LogServer not in local domain", YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("Localhost: " + localHostname.toLower(), YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("LogServer: " + serverPath.toLower(), YD_CRITICAL);
            YD_ADDISSUE("LogServer not in local domain", YD_CRITICAL);
            YD_RESULT_ENDSECTION
            return;
        }
    }

    QUrlQuery data;
    QNetworkReply::NetworkError net_error;
    int http_status=0;

    // Create temporary instance for testing the logserver connection
    NetLogger testLogger;

    // Read the full server path entered into the UI control again. The path returned from the DNS
    // might differ if an DNS alias is used. In this case, the certificate will be incorrect.
    serverPath         =logServerAddress;
    QString apiKey     =logServerAPIKey;
    QString name       ="ConnectionTest";
    QString errorString="n/a";

    testLogger.configure(serverPath, EventInfo::SourceType::RDS, name, apiKey, true);
    data.addQueryItem("api_key",apiKey);
    bool success=testLogger.postData(data, NETLOG_ENDPT_TEST, net_error, http_status, errorString);
    if (!success)
    {
        if (http_status==0)
        {
            YD_ADDRESULT_COLORLINE("No response from LogServer (Error: "+errorString+")", YD_CRITICAL);
            switch (net_error)
            {
            case QNetworkReply::NetworkError::SslHandshakeFailedError:
                YD_ADDRESULT_COLORLINE("Is SSL certificate installed / valid?", YD_CRITICAL);
                break;
            case QNetworkReply::NetworkError::AuthenticationRequiredError:
                YD_ADDRESULT_COLORLINE("Is API key correct?", YD_CRITICAL);
                break;
            default:
                YD_ADDRESULT_COLORLINE("Is server running? Is port correct?", YD_CRITICAL);
                break;
            }
            YD_ADDISSUE("No response from LogServer", YD_CRITICAL);
        }
        else
        {
            YD_ADDRESULT_COLORLINE("LogServer rejected request (Status: "+QString::number(http_status)+")", YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("Is API key corrent?", YD_CRITICAL);
            YD_ADDISSUE("LogServer rejected request", YD_CRITICAL);
        }
    }
    else
    {
        YD_ADDRESULT_COLORLINE("LogServer responded", YD_SUCCESS);
    }

    YD_RESULT_ENDSECTION
}
