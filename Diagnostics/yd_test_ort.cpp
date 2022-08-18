#include "yd_test_ort.h"
#include "yd_global.h"
#include "yd_helper.h"
#include "../Client/rds_exechelper.h"


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
            YD_ADDRESULT_LINE("Seed server " + connectIP + " responded to ping");
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
                YD_ADDRESULT_LINE("Fallback seed server " + fallbackConnectIP + " responded to ping");
            }
            else
            {
                YD_ADDRESULT_COLORLINE("Unable to ping fallback seed server " + fallbackConnectIP, YD_CRITICAL);
                YD_ADDISSUE("Unable to ping fallback seed server " + fallbackConnectIP, YD_CRITICAL);
            }
        }
    }

    YD_RESULT_ENDSECTION

    // Try connecting to the seed server
    if (!ortConfig.ortConnectCmd.isEmpty())
    {
        if (!mountServerAndVerify(ortConfig.ortConnectCmd, issues, results))
        {
            // TODO
        }
    }

    // Try connecting to the fallback seed server
    if (!ortConfig.ortConnectCmd.isEmpty())
    {
        if (!mountServerAndVerify(ortConfig.ortFallbackConnectCmd, issues, results))
        {
            // TODO
        }
    }

    // Now try to connect to all servers defined in the server list
    for (int i=0; i<serverList.servers.count(); i++)
    {
//        if (!mountServerAndVerify(ortConfig.ortFallbackConnectCmd, issues, results))
//        {
//            // TODO
//        }

    }

}


bool ydTestORT::mountServerAndVerify(QString connectCmd, QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION

    if (connectCmd!="")
    {
        rdsExecHelper execHelper;
        execHelper.setMonitorNetUseOutput();
        execHelper.setCommand(connectCmd);

        QString serverName = ydHelper::extractIP(connectCmd);
        YD_ADDRESULT_LINE("Connecting to server " + serverName);
        YD_ADDRESULT_LINE("Command: " + connectCmd);

        if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
        {
            YD_ADDRESULT_COLORLINE("Connecting to server failed " + serverName, YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("Detected error: " + execHelper.getDetectedNetUseErrorMessage(), YD_CRITICAL);
        }
    }

    // TODO: Read server list if it's one of the seed servers

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
        }
    }

    YD_RESULT_ENDSECTION

    return true;
}
