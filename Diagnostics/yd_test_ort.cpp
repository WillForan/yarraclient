#include "yd_test_ort.h"
#include "yd_global.h"
#include "yd_helper.h"
#include "../Client/rds_exechelper.h"

#include <QDir>


ydTestORT::ydTestORT() : ydTest()
{
}


QString ydTestORT::getName()
{
    return "Offline Reconstruction";
}


QString ydTestORT::getDescription()
{
    return "indentify ORT-related problems";
}


bool ydTestORT::run(QString& issues, QString& results)
{
    serverList.clearList();

    YD_RESULT_STARTSECTION
    if (broker->contains(YD_KEY_NO_ORT_AVAILABLE))
    {
        YD_ADDRESULT("Skipping ORT tests (not configured)");
        YD_RESULT_ENDSECTION
        return true;
    }

    ortConfig.loadConfiguration();
    if (!ortConfig.isConfigurationValid())
    {
        YD_ADDRESULT_COLORLINE("ORT configuration file is invalid", YD_CRITICAL);
        YD_ADDISSUE("ORT configuration file invalid",YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return true;
    }

    YD_ADDRESULT("ORT system name: " + ortConfig.ortSystemName);
    if ((ortConfig.ortSystemName.isEmpty()) || (ortConfig.ortSystemName=="NotGiven"))
    {
        YD_ADDRESULT_COLORLINE("ORT system name has not been defined", YD_WARNING);
        YD_ADDISSUE("ORT system name not defined",YD_WARNING);
    }

    if ((!ortConfig.logServerAddress.isEmpty()) && (!ortConfig.logServerAPIKey.isEmpty()))
    {
        broker->insert("logserver_address", ortConfig.logServerAddress);
        broker->insert("logserver_apikey", ortConfig.logServerAPIKey);
        broker->insert("logserver_source", "ORT");
    }

    YD_RESULT_ENDSECTION

    testConnectivity(issues, results);

    return true;
}


void ydTestORT::testConnectivity(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION

    if (ortConfig.ortConnectCmd.isEmpty())
    {
        YD_ADDRESULT_COLORLINE("ORT connect command has not been defined", YD_WARNING);
        YD_ADDRESULT_COLORLINE("ORT will not work", YD_WARNING);
        YD_ADDISSUE("ORT connect command not defined", YD_CRITICAL);
        return;
    }

    // Check if the ORT server path already exists. If it does, try to disconnect
    {
        QDir serverDir;
        if (serverDir.exists(ortConfig.ortServerPath))
        {
            qDebug() << serverDir.absolutePath();

            YD_ADDRESULT_COLORLINE("Server path already mapped " + ortConfig.ortServerPath, YD_WARNING);
            YD_ADDRESULT_COLORLINE("Trying to disconnect", YD_WARNING);
            YD_ADDISSUE("Server path already mapped", YD_WARNING);

            if (ortConfig.ortDisconnectCmd!="")
            {
                rdsExecHelper execHelper;
                execHelper.setMonitorNetUseOutput();
                execHelper.setCommand(ortConfig.ortDisconnectCmd);

                YD_ADDRESULT_LINE("Command: " + ortConfig.ortDisconnectCmd);

                if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
                {
                    YD_ADDRESULT_COLORLINE("Calling the disconnect command failed", YD_CRITICAL);
                    YD_ADDRESULT_COLORLINE("Detected error: " + execHelper.getDetectedNetUseErrorMessage(), YD_CRITICAL);
                    YD_ADDISSUE("Disconnecting failed ", YD_CRITICAL);
                }
            }
        }
    }

    // First ping the seed and seed-fallback server
    QString connectIP = ydHelper::extractIP(ortConfig.ortConnectCmd);
    if (connectIP.isEmpty())
    {
        YD_ADDRESULT_LINE("Unknown connect command for ORT seed server");
        YD_ADDISSUE("Unknown connect command for ORT seed server", YD_INFO);
    }
    else
    {
        if (ydHelper::pingServer(connectIP))
        {
            YD_ADDRESULT_COLORLINE("Seed server " + connectIP + " responded to ping", YD_SUCCESS);
        }
        else
        {
            YD_ADDRESULT_COLORLINE("Unable to ping seed server " + connectIP, YD_CRITICAL);
            YD_ADDISSUE("Unable to ping seed server " + connectIP, YD_CRITICAL);
        }
    }

    QString fallbackConnectIP = ortConfig.ortFallbackConnectCmd;
    if (!fallbackConnectIP.isEmpty())
    {
        fallbackConnectIP=ydHelper::extractIP(fallbackConnectIP);
        if (fallbackConnectIP.isEmpty())
        {
            YD_ADDRESULT_LINE("Unknown connect command for ORT fallback seed server");
            YD_ADDISSUE("Unknown connect command for ORT fallback seed server", YD_INFO);
        }
        else
        {
            if (ydHelper::pingServer(fallbackConnectIP))
            {
                YD_ADDRESULT_COLORLINE("Fallback seed server " + fallbackConnectIP + " responded to ping", YD_SUCCESS);
            }
            else
            {
                YD_ADDRESULT_COLORLINE("Unable to ping fallback seed server " + fallbackConnectIP, YD_CRITICAL);
                YD_ADDISSUE("Unable to ping fallback seed server " + fallbackConnectIP, YD_CRITICAL);
            }
        }
    }

    YD_RESULT_ENDSECTION
    YD_RESULT_STARTSECTION
    YD_ADDRESULT("Testing server connectivity...");
    YD_RESULT_ENDSECTION

    // Try connecting to the seed server
    if (!ortConfig.ortConnectCmd.isEmpty())
    {
        YD_RESULT_STARTSECTION
        mountServerAndVerify(ortConfig.ortConnectCmd, issues, results, true);
        YD_RESULT_ENDSECTION
    }

    // Try connecting to the fallback seed server
    if (!ortConfig.ortConnectCmd.isEmpty())
    {
        YD_RESULT_STARTSECTION
        mountServerAndVerify(ortConfig.ortFallbackConnectCmd, issues, results, true);
        YD_RESULT_ENDSECTION
    }

    YD_RESULT_STARTSECTION
    if (serverList.servers.isEmpty())
    {
        YD_ADDRESULT_COLORLINE("Server list is empty", YD_INFO);
    }
    else
    {
        YD_ADDRESULT_LINE("Available servers:");
        for (int i=0; i<serverList.servers.count(); i++)
        {
            YD_ADDRESULT_LINE(QString(" - Name = ") + serverList.servers.at(i)->name + QString(", Type = ") + serverList.servers.at(i)->type.join(","));
        }
    }
    YD_RESULT_ENDSECTION

    // Now try to connect to all servers defined in the server list
    for (int i=0; i<serverList.servers.count(); i++)
    {
        YD_RESULT_STARTSECTION

        // First, try to ping server
        QString connectIP = ydHelper::extractIP(serverList.servers.at(i)->connectCmd);
        if (connectIP.isEmpty())
        {
            YD_ADDRESULT_LINE("Unknown connect command for ORT server " + serverList.servers.at(i)->name);
            YD_ADDISSUE("Unknown connect command for ORT server " + serverList.servers.at(i)->name , YD_INFO);
        }
        else
        {
            if (ydHelper::pingServer(connectIP))
            {
                YD_ADDRESULT_COLORLINE("Server " + connectIP + " responded to ping", YD_SUCCESS);
            }
            else
            {
                YD_ADDRESULT_COLORLINE("Unable to ping server " + serverList.servers.at(i)->name, YD_CRITICAL);
                YD_ADDISSUE("Unable to ping server " + serverList.servers.at(i)->name, YD_CRITICAL);
            }
        }

        // Now try to mount the server
        mountServerAndVerify(serverList.servers.at(i)->connectCmd, issues, results, false, serverList.servers.at(i)->name);

        YD_RESULT_ENDSECTION
    }
}


bool ydTestORT::mountServerAndVerify(QString connectCmd, QString& issues, QString& results, bool isSeedServer, QString serverName)
{

    if (serverName.isEmpty())
    {
        if (!connectCmd.isEmpty())
        {
            serverName = ydHelper::extractIP(connectCmd);
        }
    }

    if (connectCmd!="")
    {
        rdsExecHelper execHelper;
        execHelper.setMonitorNetUseOutput();
        execHelper.setCommand(connectCmd);

        YD_ADDRESULT_LINE("Connecting to server " + serverName);
        YD_ADDRESULT_LINE("Command: " + connectCmd);

        if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
        {
            YD_ADDRESULT_COLORLINE("Connecting to server failed " + serverName, YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("Detected error: " + execHelper.getDetectedNetUseErrorMessage(), YD_CRITICAL);
            YD_ADDISSUE("Connecting to server failed " + serverName, YD_CRITICAL);
        }
    }

    QDir serverDir;
    serverDir.cd(ortConfig.ortServerPath);
    serverDir.refresh();
    if (!serverDir.exists())
    {
        YD_ADDRESULT_COLORLINE("Unable to map server folder " + ortConfig.ortServerPath, YD_CRITICAL);
        YD_ADDISSUE("Unable to map server folder " + ortConfig.ortServerPath, YD_CRITICAL);
    }
    else
    {
        if ((!serverDir.exists(ORT_MODEFILE)) || (!serverDir.exists(ORT_SERVERFILE)))
        {
            YD_ADDRESULT_COLORLINE("Mode or server file not found at " + ortConfig.ortServerPath, YD_CRITICAL);
            YD_ADDISSUE("Mode or server file not found on server " + serverName, YD_CRITICAL);
        }
        else
        {
            YD_ADDRESULT_COLORLINE("Server and mode file found", YD_SUCCESS);

            // Read server list if it's one of the seed servers
            if (isSeedServer)
            {
                if (!serverDir.exists(ORT_SERVERLISTFILE))
                {
                    YD_ADDRESULT_COLORLINE("No server list found", YD_INFO);
                }
                else
                {
                    YD_ADDRESULT_COLORLINE("Server list found", YD_SUCCESS);
                    if ((serverList.servers.isEmpty()))
                    {
                        YD_ADDRESULT_LINE("Reading server list");
                        serverList.readServerList(serverDir.absolutePath());
                    }
                }
            }

            // TODO: Check for available disk space and warn if low
            // TODO: Test write permissions
        }
    }

    if (ortConfig.ortDisconnectCmd!="")
    {
        rdsExecHelper execHelper;
        execHelper.setMonitorNetUseOutput();
        execHelper.setCommand(ortConfig.ortDisconnectCmd);

        YD_ADDRESULT_LINE("Disconnecting from server");
        YD_ADDRESULT_LINE("Command: " + ortConfig.ortDisconnectCmd);

        if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
        {
            YD_ADDRESULT_COLORLINE("Calling the disconnect command failed", YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("Detected error: " + execHelper.getDetectedNetUseErrorMessage(), YD_CRITICAL);
            YD_ADDISSUE("Disconnecting from server failed ", YD_CRITICAL);
        }
    }

    YD_RESULT_ENDSECTION

    return true;
}
