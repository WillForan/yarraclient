#include <QtWidgets>

#include "sac_network.h"
#include "sac_global.h"
#include "../Client/rds_exechelper.h"
#include "../Client/rds_network.h"


sacNetwork::sacNetwork()
{
    connectCmd="";
    serverPath="";
    disconnectCmd="";
    defaultNotification="";
    preferredMode="";
    copyErrorMsg="";
    showConfigurationAfterError=false;
}


bool sacNetwork::readConfiguration()
{
    appPath=qApp->applicationDirPath();

    {
        QSettings config(appPath+"/sac.ini", QSettings::IniFormat);

        serverPath =    config.value("Configuration/ServerPath","").toString();
        connectCmd =    config.value("Configuration/ConnectCmd","").toString();
        disconnectCmd = config.value("Configuration/DisconnectCmd","").toString();
        systemName =    config.value("Configuration/Name","Unknown").toString();
        defaultNotification = config.value("Configuration/DefaultNotification","").toString();
        preferredMode = config.value("Configuration/PreferredMode","").toString();

        if (serverPath.length()==0)
        {
            return false;
        }

        return true;
    }
}


void sacNetwork::writeConfiguration()
{
    {
        QSettings config(appPath+"/sac.ini", QSettings::IniFormat);

        config.setValue("Configuration/ServerPath",serverPath);
        config.setValue("Configuration/ConnectCmd",connectCmd);
        config.setValue("Configuration/DisconnectCmd",disconnectCmd);
        config.setValue("Configuration/Name",systemName);
        config.setValue("Configuration/DefaultNotification",defaultNotification);
        config.setValue("Configuration/PreferredMode",preferredMode);
    }
}


bool sacNetwork::openConnection(bool isConsole)
{
    if (connectCmd!="")
    {
        rdsExecHelper execHelper;
        execHelper.setCommand(connectCmd);

        if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
        {
            RTI->log("Calling the connect command failed: " + connectCmd);
        }
    }

    QString errorMessage="";
    bool error=false;

    if (serverPath=="")
    {
        RTI->log("ERROR: Server path has not been defined.");

        errorMessage="The Yarra server path has not been defined.\nPlease check the configuration.";
        error=true;
    }

    if (!error && !serverDir.exists(serverPath))
    {
        RTI->log("ERROR: Could not access Yarra server path: " + serverPath);
        RTI->log("ERROR: Check if network drive has been mounted correctly. Or check configuration.");

        errorMessage="Could not access the path of the Yarra server.\nPlease make sure that the network drive can be \nmapped and that the configuration is valid.";
        error=true;
    }

    if (!error && !serverDir.cd(serverPath))
    {
        RTI->log("ERROR: Could not change to base path of network drive.");
        RTI->setSevereErrors(true);

        errorMessage="Could not enter the Yarra server directory.\nPlease make sure that the network drive can be\nmapped and that the configuration is valid.";

        error=true;
    }

    if (!error && (!serverDir.exists(ORT_MODEFILE) || !serverDir.exists(ORT_SERVERFILE)))
    {
        RTI->log("ERROR: ORT Mode file not found.");
        RTI->setSevereErrors(true);

        errorMessage="Unable to connect to server.\n\nCould not find Yarra configuration files (YarraServer.cfg/YarraModes.cfg) at expected location.\n\nPlease verify the network configuration of the Yarra client and server.";
        error=true;
    }

    // Show error message and terminate the client
    if (error && !isConsole)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText(errorMessage+"\n\nDo you want to review the configuration?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setWindowIcon(ORT_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        if (msgBox.exec()==QMessageBox::Yes)
        {
            showConfigurationAfterError=true;
        }
    }
    return !error;
}


void sacNetwork::closeConnection()
{
    if (disconnectCmd!="")
    {
        rdsExecHelper execHelper;
        execHelper.setCommand(disconnectCmd);

        if (!execHelper.callNetUseTimout(ORT_CONNECT_TIMEOUT))
        {
            RTI->log("Calling the disconnect command failed: " + disconnectCmd);
        }
    }
}


bool sacNetwork::fileExistsOnServer(QString filename)
{
    serverDir.refresh();
    QString fullFileName=serverDir.absoluteFilePath(filename);
    return (QFile::exists(fullFileName));
}


bool sacNetwork::copyMeasurementFile(QString sourceFile, QString targetFile)
{
    copyErrorMsg="";
    serverDir.refresh();

    QString sourceName=sourceFile;
    QString destName=serverDir.absoluteFilePath(targetFile);

    RTI->log("Transferring measurement to server.");
    RTI->log("Source: " + sourceName);
    RTI->log("Target: " + destName);

    // Check free diskspace on the network drive
    QFileInfo srcinfo(sourceName);
    qint64 diskSpace=RTI->getFreeDiskSpace(serverDir.absolutePath());
    RTI->debug("Disk space on netdrive = " + QString::number(diskSpace));
    RTI->debug("File size is = " +  QString::number(srcinfo.size()));

    if (srcinfo.size() > diskSpace)
    {
        RTI->log("ERROR: Not enough diskspace on network drive to transfer file.");
        RTI->log("ERROR: File size = " + QString::number(srcinfo.size()));
        RTI->log("ERROR: Remaining disk space = " + QString::number(diskSpace));
        RTI->log("Canceling transfer.");
        copyErrorMsg="Not enough diskspace on server available.";

        return false;
    }

    if (QFile::exists(destName))
    {
        RTI->log("WARNING: File already exists in target folder: " + destName);
        RTI->log("Skipping transfer.");
        copyErrorMsg="Filename already exists on server. Select different TaskID.";

        return false;
    }

    bool copyError=false;

    // Use separate copy thread and local event loop to keep the application
    // responsive while the data is transferred. This is very important because
    // otherwise problems might arise if the qsingleapplication interface
    // receives messages while it was blocked (might lead to infinite loops)
    {
        rdsCopyThread copyThread;
        copyThread.sourceName=sourceName;
        copyThread.destName=destName;

        QEventLoop q;
        connect(&copyThread, SIGNAL(finished()), &q, SLOT(quit()));

        QTime ti;
        ti.start();

        copyThread.start();
        q.exec();

        // Check if the event loop returned too early.
        if (!copyThread.finishedCopy)
        {
            RTI->log("WARNING: QEventLoop returned too early during copy. Starting secondary loop.");

            // Wait until copying has finished, but restrict the waiting to 1 hour max
            while ((!copyThread.isFinished()) && (ti.elapsed()<RDS_COPY_TIMEOUT))
            {
                RTI->processEvents();
                Sleep(RDS_SLEEP_INTERVAL);
            }

            if (!copyThread.finishedCopy)
            {
                RTI->log("ERROR: Second copy wait loop timed out! Copying file failed.");
                copyError=true;
            }
        }

        copyError=!copyThread.success;
    }

    if (copyError)
    {
        RTI->log("ERROR: Error copying the file!");
        RTI->log("ERROR: Source = " + sourceName);
        RTI->log("ERROR: Destination = " + destName);

        copyErrorMsg="Error while copying file.";
        return false;
    }

    return true;
}
