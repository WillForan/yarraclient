#include "yd_test_rds.h"

#include "yd_global.h"
#include "yd_helper.h"
#include "../Client/rds_exechelper.h"

#include <QDir>


ydTestRDS::ydTestRDS() : ydTest()
{
}


QString ydTestRDS::getName()
{
    return "Raw Data Storage";
}


QString ydTestRDS::getDescription()
{
    return "indentify RDS-related problems";
}


bool ydTestRDS::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION

    if (broker->contains(YD_KEY_NO_RDS_AVAILABLE))
    {
        YD_ADDRESULT("Skipping RDS tests (not configured)");
        YD_RESULT_ENDSECTION
        return true;
    }

    rdsConfig.loadConfiguration();
    if (!rdsConfig.isConfigurationValid())
    {
        YD_ADDRESULT_COLORLINE("RDS configuration file is invalid", YD_CRITICAL);
        YD_ADDISSUE("RDS configuration file invalid", YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return true;
    }

    YD_ADDRESULT("RDS system name: " + rdsConfig.infoName);
    if ((rdsConfig.infoName.isEmpty()) || (rdsConfig.infoName=="NotGiven"))
    {
        YD_ADDRESULT_COLORLINE("RDS system name has not been defined", YD_WARNING);
        YD_ADDISSUE("RDS system name not defined", YD_WARNING);
    }

    if ((!rdsConfig.logServerPath.isEmpty()) && (!rdsConfig.logApiKey.isEmpty()))
    {
        broker->insert("logserver_address", rdsConfig.logServerPath);
        broker->insert("logserver_apikey", rdsConfig.logApiKey);
        broker->insert("logserver_source", "RDS");
    }

    // Open connection and check for storage destination
    if (rdsConfig.netDriveReconnectCmd.isEmpty())
    {
        YD_ADDRESULT_LINE("No RDS connect command defined");
    }
    else
    {
        // First ping the seed and seed-fallback server
        QString connectIP = ydHelper::extractIP(rdsConfig.netDriveReconnectCmd);
        if (connectIP.isEmpty())
        {
            YD_ADDRESULT_LINE("Unknown connect command for RDS storage");
            YD_ADDISSUE("Unknown connect command for RDS storage", YD_INFO);
        }
        else
        {
            if (ydHelper::pingServer(connectIP))
            {
                YD_ADDRESULT_COLORLINE("RDS storage server " + connectIP + " responded to ping", YD_SUCCESS);
            }
            else
            {
                YD_ADDRESULT_COLORLINE("Unable to ping RDS server " + connectIP, YD_CRITICAL);
                YD_ADDISSUE("Unable to ping RDS server " + connectIP, YD_CRITICAL);
                YD_RESULT_ENDSECTION
                return true;
            }
        }

        rdsExecHelper execHelper;
        execHelper.setMonitorNetUseOutput();
        execHelper.setCommand(rdsConfig.netDriveReconnectCmd);

        YD_ADDRESULT_LINE("Running RDS connect command");
        YD_ADDRESULT_LINE("Command: " + rdsConfig.netDriveReconnectCmd);

        if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
        {
            YD_ADDRESULT_COLORLINE("Connecting to RDS storage server", YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("Detected error: " + execHelper.getDetectedNetUseErrorMessage(), YD_CRITICAL);
            YD_ADDISSUE("Connecting to RDS storage failed", YD_CRITICAL);
            YD_RESULT_ENDSECTION
            return true;
        }
    }

    QDir serverDir;
    if (!serverDir.cd(rdsConfig.netDriveBasepath))
    {
        YD_ADDRESULT_COLORLINE("Unable to map RDS storage folder " + rdsConfig.netDriveBasepath, YD_CRITICAL);
        YD_ADDISSUE("Unable to map server folder " + rdsConfig.netDriveBasepath, YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return true;
    }
    else
    {
        YD_ADDRESULT_COLORLINE("Found RDS storage folder " + rdsConfig.netDriveBasepath, YD_SUCCESS);
    }


    // Check for available disk space and warn if low
    QStorageInfo storageInfo(rdsConfig.netDriveBasepath);
    if (storageInfo.isReadOnly())
    {
        YD_ADDRESULT_COLORLINE("RDS storage folder seems read-only " + rdsConfig.netDriveBasepath, YD_WARNING);
    }

    double freeSpaceGB = storageInfo.bytesTotal()/1000./1000./1000.;
    YD_ADDRESULT_LINE("Free space on RDS storage: " + QString::number(freeSpaceGB) + " GB");

    if (freeSpaceGB < YD_LOWSPACE_ALERT)
    {
        YD_ADDRESULT_COLORLINE("Disk space on RDS storage critically low: " + QString::number(freeSpaceGB) + " GB", YD_CRITICAL);
        YD_ADDISSUE("Critically low disk space on RDS storage", YD_CRITICAL);
    }
    else
    {
        if (freeSpaceGB < YD_LOWSPACE_WARNING)
        {
            YD_ADDRESULT_COLORLINE("Low disk space on RDS storage: " + QString::number(freeSpaceGB) + " GB", YD_WARNING);
            YD_ADDISSUE("Low disk space on RDS storage", YD_WARNING);
        }
    }

    // Test write permissions
    QString uidFilename = QUuid::createUuid().toString();
    uidFilename.remove(0,1);
    uidFilename.remove(uidFilename.length()-1,1);
    uidFilename += ".diagnostics";

    QFile testFile(serverDir.absoluteFilePath(uidFilename));
    if (!testFile.open(QIODevice::ReadWrite))
    {
        YD_ADDRESULT_COLORLINE("Unable to create files on RDS storage", YD_CRITICAL);
        YD_ADDISSUE("Unable to create files on RDS storage", YD_CRITICAL);
    }
    else
    {
        char testString[] = "Created by Yarra Diagnostics";
        if (testFile.write(testString) < 0)
        {
            YD_ADDRESULT_COLORLINE("Unable to write on RDS storage", YD_CRITICAL);
            YD_ADDISSUE("Unable to write on RDS storage", YD_CRITICAL);
        }

        // Remove test file again
        if (!testFile.remove())
        {
            YD_ADDRESULT_COLORLINE("Incorrect file permissions for RDS storage", YD_CRITICAL);
            YD_ADDISSUE("Incorrect file permissions for RDS storage", YD_CRITICAL);
        }
        else
        {
            YD_ADDRESULT_COLORLINE("Can create and write files", YD_SUCCESS);
        }
    }

    // Disconnect from RDS storage if configured
    if (rdsConfig.netDriveDisconnectCmd.isEmpty())
    {
        YD_ADDRESULT_LINE("No RDS disconnect command defined");
    }
    else
    {
        rdsExecHelper execHelper;
        execHelper.setMonitorNetUseOutput();
        execHelper.setCommand(rdsConfig.netDriveDisconnectCmd);

        YD_ADDRESULT_LINE("Disconnecting from RDS storage");
        YD_ADDRESULT_LINE("Command: " + rdsConfig.netDriveDisconnectCmd);

        if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
        {
            YD_ADDRESULT_COLORLINE("Calling RDS disconnect command failed", YD_CRITICAL);
            YD_ADDRESULT_COLORLINE("Detected error: " + execHelper.getDetectedNetUseErrorMessage(), YD_CRITICAL);
            YD_ADDISSUE("Disconnecting from RDS storage failed ", YD_CRITICAL);
            YD_RESULT_ENDSECTION
            return true;
        }
    }

    YD_RESULT_ENDSECTION

    return true;
}

