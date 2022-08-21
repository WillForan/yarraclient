#include "yd_test_logserver.h"
#include "yd_global.h"
#include "yd_helper.h"
#include "../Client/rds_exechelper.h"


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

//    QString serverPath=ui->logServerAddressEdit->text();
//    int serverPort=8080;

//    // Check if the entered address contains a port specifier
//    int colonPos=serverPath.indexOf(":");

//    if (colonPos!=-1)
//    {
//        int cutChars=serverPath.length()-colonPos;

//        QString portString=serverPath.right(cutChars-1);
//        serverPath.chop(cutChars);

//        // Convert port string into number and validate
//        bool portValid=false;

//        int tempPort=portString.toInt(&portValid);

//        if (portValid)
//        {
//            serverPort=tempPort;
//        }
//    }

//    QString localHostname="";
//    QString localIP="";

//    // Open socket connection to see if the server is active and to
//    // determine the local IP address used for routing to the server.
//    QTcpSocket socket;
//    socket.setProxy(QNetworkProxy::NoProxy);
//    socket.connectToHost(serverPath, serverPort);

//    if (socket.waitForConnected(10000))
//    {
//        localIP=socket.localAddress().toString();
//    }
//    else
//    {
//        output += errorPrefix + "Unable to connect to server.";
//        error=true;
//    }

//    socket.disconnectFromHost();

//#ifndef NETLOGGER_DISABLE_DOMAIN_VALIDATION

//    // Lookup the hostname of the local client from the DNS server
//    if (!error)
//    {
//        localHostname=NetLogger::dnsLookup(localIP);

//        if (localHostname.isEmpty())
//        {
//            output += errorPrefix + "Unable to resolve local hostname.<br /><br />IP = " + localIP;
//            output += "<br />Check local DNS server settings.";
//            error=true;
//        }
//    }

//    // Lookup the hostname of the log server from the DNS server
//    if (!error)
//    {
//        serverPath=NetLogger::dnsLookup(serverPath);

//        if (serverPath.isEmpty())
//        {
//            output += errorPrefix + "Unable to resolve server name.<br /><br />";
//            output += "Check local DNS server settings.";
//            error=true;
//        }
//    }

//    // Compare if the local system and the server are on the same domain
//    if (!error)
//    {
//        QStringList localHostString =localHostname.toLower().split(".");
//        QStringList serverHostString=serverPath.toLower().split(".");

//        if ((localHostString.count()<2) || (serverHostString.count()<2))
//        {
//            output += errorPrefix + "Error resolving hostnames.<br /><br />";
//            output += "Local host: " + localHostname.toLower() + " ("+localIP+")<br />";
//            output += "Log server: " + serverHostString.join(".");
//            error=true;
//        }
//        else
//        {
//            if ((localHostString.at(localHostString.count()-1)!=(serverHostString.at(serverHostString.count()-1)))
//               || (localHostString.at(localHostString.count()-2)!=(serverHostString.at(serverHostString.count()-2))))
//            {
//                output += errorPrefix + "Server not in local domain.<br /><br />";
//                output += "Local host: " + localHostname.toLower() + "<br />";
//                output += "Log server: " + serverPath.toLower() + "<br />";
//                error=true;
//            }
//        }
//    }

//#endif

//    // Check if the server responds to the test entry point
//    if (!error)
//    {
//        QUrlQuery data;
//        QNetworkReply::NetworkError net_error;
//        int http_status=0;

//        // Create temporary instance for testing the logserver connection
//        NetLogger testLogger;

//        // Read the full server path entered into the UI control again. The path returned from the DNS
//        // might differ if an DNS alias is used. In this case, the certificate will be incorrect.
//        serverPath         =ui->logServerAddressEdit->text();
//        QString apiKey     =ui->logServerAPIKeyEdit->text();
//        QString name       ="ConnectionTest";
//        QString errorString="n/a";

//        testLogger.configure(serverPath, EventInfo::SourceType::RDS, name, apiKey, true);
//        data.addQueryItem("api_key",apiKey);
//        bool success=testLogger.postData(data, NETLOG_ENDPT_TEST, net_error, http_status, errorString);

//        if (!success)
//        {
//            if (http_status==0)
//            {
//                output += errorPrefix + "No response from server (Error: "+errorString+").<br /><br />";

//                switch (net_error)
//                {
//                case QNetworkReply::NetworkError::SslHandshakeFailedError:
//                    output += "Is SSL certificate installed / valid?";
//                    break;
//                case QNetworkReply::NetworkError::AuthenticationRequiredError:
//                    output += "Is API key correct?";
//                    break;
//                default:
//                    output += "Is server running? Is port correct?";
//                    break;
//                }
//            }
//            else
//            {
//                output += errorPrefix + "Server rejected request (Status: "+QString::number(http_status)+").<br /><br />Is API key corrent?";
//            }

//            error=true;
//        }
//    }


    YD_RESULT_ENDSECTION
}
