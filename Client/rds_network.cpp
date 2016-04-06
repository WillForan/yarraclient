#include <QtCore/qt_windows.h>
#if defined(Q_OS_WIN)
    #include <QtCore/QLibrary>
    #include <QtCore/qt_windows.h>
#endif
#if defined(Q_OS_UNIX)
    #include <time.h>
#endif

#include "rds_network.h"
#include "rds_global.h"

#ifdef YARRA_APP_RDS
    #include "rds_checksum.h"
    #include "rds_copydialog.h"
#endif


rdsNetwork::rdsNetwork()
{
    queueDir.setCurrent(RTI->getAppPath() + "/" + RDS_DIR_QUEUE);
    queueDir.setFilter(QDir::Files);

    currentFilename ="";
    currentProt     ="";
    currentFilesize =-1;
    currentTimeStamp="";

    connectionActive=false;
}


rdsNetwork::~rdsNetwork()
{
}



bool rdsNetwork::openConnection()
{
    queueDir.refresh();

    // Either open FTP connection or check connection to network drive

    if (RTI_CONFIG->isNetworkModeDrive())
    {
        // Call reconnect cmd entered into configuration
        runReconnectCmd();

        // Test if network path exists
        if (!networkDrive.exists(RTI_CONFIG->netDriveBasepath))
        {
            // If the network base path should not be created (default), abort with error message
            if (!RTI_CONFIG->netDriveCreateBasepath)
            {
                RTI->log("Error: Could not access network drive path: " + RTI_CONFIG->netDriveBasepath);
                RTI->log("Error: Check if network drive has been mounted. Or check configuration.");
                return false;
            }
            else
            {
                // If the option is enabled, try to create it
                if (!networkDrive.mkpath(RTI_CONFIG->netDriveBasepath))
                {
                    RTI->log("Error: Unable to create network drive path: " + RTI_CONFIG->netDriveBasepath);
                    RTI->log("Error: Check configuration and network permissions.");
                    return false;
                }
                else
                {
                    if (!networkDrive.exists(RTI_CONFIG->netDriveBasepath))
                    {
                        RTI->log("Error: Creating the network drive path was not successful: " + RTI_CONFIG->netDriveBasepath);
                        RTI->log("Error: Check configuration and network permissions.");
                        return false;
                    }
                }
            }
        }

        // Change to network drive
        RTI->debug("Network base path is :" + RTI_CONFIG->netDriveBasepath);
        if (!networkDrive.cd(RTI_CONFIG->netDriveBasepath))
        {
            RTI->log("ERROR: Could not change to base path of network drive.");
            RTI->setSevereErrors(true);
            return false;
        }

        // Look if the folder for the system exists, if not create it
        if (!networkDrive.exists(RTI_CONFIG->infoName))
        {
            RTI->debug("Target folder does not exist. Creating it: " + RTI_CONFIG->infoName);

            if (!networkDrive.mkdir(RTI_CONFIG->infoName))
            {
                RTI->log("Error: Error creating system folder on network drive.");
                RTI->log("Error: Rights for writing and creating files might not exist.");
                RTI->log("Error: Check configuration of network drive.");
                RTI->setSevereErrors(true);
                return false;
            }
        }

        networkDrive.cd(RTI_CONFIG->infoName);
        RTI->debug("Selected storage path is :" + networkDrive.absolutePath());
    }

    if (RTI_CONFIG->isNetworkModeFTP())
    {
        // TODO
        //ftp.connectToHost(RTI_CONFIG->netFTPIP);
        //ftp.login(RTI_CONFIG->netFTPUser, RTI_CONFIG->netFTPPassword);
        //ftp.cd(RTI_CONFIG->netFTPBasepath);

        return false;
    }

    connectionActive=true;
    return true;
}


void rdsNetwork::closeConnection()
{  
    // NOTE: Nothing to be done for the network mode.

    if (RTI_CONFIG->isNetworkModeFTP())
    {
        //ftp.close();
    }

    connectionActive=false;
}


bool rdsNetwork::transferFiles()
{
#ifdef YARRA_APP_RDS
    rdsCopyDialog copyDialog;
    copyDialog.show();
#endif

    // Calling getQueueCount will also refresh the queue directory list.
    if (getQueueCount()==0)
    {
        return true;
    }

    // Read files in directory
    fileList=queueDir.entryList();

    bool success=true;
    for (int i=0; i<fileList.count(); i++)
    {
        success=getFileToProcess(i);

        //RTI->log("DBG: Read file info");

        // Try to copy file
        if (success)
        {
            success=copyFile();
        }

        //RTI->log("DBG: Copied file");

        // If sucessfull, verify if copied correctly
        if (success)
        {
            success=verifyTransfer();
        }

        //RTI->log("DBG: Verified");

        // If sucessfull, remove file from queue directory
        if (success)
        {
            success=removeFile();
        }

        //RTI->log("DBG: Deleted file");

        releaseFile();

        //RTI->log("DBG: Released file");

        RTI->processEvents();

        //RTI->log("DBG: Processed events");

    }

    //RTI->log("DBG: Left loop");

    fileList.clear();

#ifdef YARRA_APP_RDS
    copyDialog.close();
#endif

    return true;
}


int rdsNetwork::getQueueCount()
{
    queueDir.refresh();
    return queueDir.count();
}


bool rdsNetwork::isQueueEmpty()
{    
    return (getQueueCount()==0);
}


bool rdsNetwork::getFileToProcess(int index)
{
    if ((index<0) || (index>=fileList.count()))
    {
        RTI->log("Error: File transfer list is invalid!");
        return false;
    }

    currentFilename=fileList.at(index);
    RTI->log("Transfering file " + currentFilename + "...");

    if (!queueDir.exists(currentFilename))
    {
        RTI->log("Error: File could not be found: " + currentFilename);
        return false;
    }

    int sepPos=currentFilename.indexOf(RTI_SEPT_CHAR);

    if (sepPos==-1)
    {
        RTI->log("Error: File name does not contain protocol name: " + currentFilename);
        return false;
    }

    currentProt=currentFilename;
    currentProt.truncate(sepPos);
    QFileInfo fileInfo(queueDir, currentFilename);
    currentFilesize=fileInfo.size();

    return true;
}


bool rdsNetwork::copyFile()
{
    if (RTI_CONFIG->isNetworkModeDrive())
    {
        // Check if protocol directory exists
        if (!networkDrive.exists(currentProt))
        {
            if (!networkDrive.mkdir(currentProt))
            {
                RTI->log("Error: Creating protocol directory not possible: " + currentProt);
                RTI->setSevereErrors(true);
                return false;
            }
        }

        QString sourceName=queueDir.absoluteFilePath(currentFilename);
        QString destName=networkDrive.absolutePath() + "/" + currentProt + "/" + currentFilename;
        currentTimeStamp="";

        // Check free diskspace on the network drive
        QFileInfo srcinfo(sourceName);
        qint64 diskSpace=RTI->getFreeDiskSpace(networkDrive.absolutePath());
        RTI->debug("Disk space on netdrive = " + QString::number(diskSpace));
        RTI->log("Size of source file is " + QString::number(srcinfo.size()));

        if (srcinfo.size() > diskSpace)
        {
            RTI->log("ERROR: Not enough diskspace on network drive to transfer file.");
            RTI->log("ERROR: File size = " + QString::number(srcinfo.size()));
            RTI->log("ERROR: Remaining disk space = " + QString::number(diskSpace));
            RTI->log("Canceling transfer.");
            RTI->setSevereErrors(true);
            return false;
        }

        if (QFile::exists(destName))
        {
            RTI->log("WARNING: File already exists in target folder: " + destName);

            // Create a time stamp for copying the file under a different name
            currentTimeStamp="_"+QDateTime::currentDateTime().toString("ddMMyyhhmmss");

            // If a file also exists with this name, append ms to the time stamp
            if (QFile::exists(destName+currentTimeStamp))
            {
                 currentTimeStamp="_"+QDateTime::currentDateTime().toString("ddMMyyhhmmsszzz");
            }

            // If also this file exists, something must be wrong
            if (QFile::exists(destName+currentTimeStamp))
            {
                RTI->log("ERROR: Unable to create unique filename: " + destName);
                RTI->log("Something must be wrong with the file system.");
                return false;
            }

            // Add the time stamp to the file name
            destName=destName+currentTimeStamp;
            RTI->log("Appending time stamp to filename: " + destName);
        }


#ifdef YARRA_APP_RDS
        // Calculate the MD5 checksum prior to copying to diagnose sporadic file corruptions.
        if (RDS_LOG_CHECKSUM)
        {
            QString checksum=rdsChecksum::getChecksum(sourceName);
            RTI->log("Local MD5 checksum is " + checksum);
        }
#endif

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
                RTI->log("Warning: QEventLoop returned too early during copy. Starting secondary loop.");

                // Wait until copying has finished, but restrict the waiting to 1 hour max
                while ((!copyThread.isFinished()) && (ti.elapsed()<RDS_COPY_TIMEOUT))
                {
                    RTI->processEvents();
                    Sleep(RDS_SLEEP_INTERVAL);
                }

                if (!copyThread.finishedCopy)
                {
                    RTI->log("Error: Second copy wait loop timed out! Copying file failed.");
                    copyError=true;
                }
            }

            copyError=!copyThread.success;
        }

        //copyError=!QFile::copy(sourceName,destName);

        if (copyError)
        {
            RTI->log("Error: Error copying the file!");
            RTI->log("Error: Source = " + sourceName);
            RTI->log("Error: Destination = " + destName);
            RTI->setSevereErrors(true);
            return false;
        }

    }

    if (RTI_CONFIG->isNetworkModeFTP())
    {
        // TODO
    }

    return true;
}


bool rdsNetwork::verifyTransfer()
{
    if (RTI_CONFIG->isNetworkModeDrive())
    {
        // currentTimeStamp is empty unless a file with the same name already existed in the target folder
        QString destName=networkDrive.absolutePath() + "/" + currentProt + "/" + currentFilename + currentTimeStamp;
        QFileInfo fileInfo(destName);

        if (!fileInfo.exists())
        {
            RTI->log("Error: Copied file does not exist at target position: " + destName);
            RTI->log("Error: Transferring the file was not successful.");
            RTI->setSevereErrors(true);
            return false;
        }

        if (fileInfo.size() != currentFilesize)
        {
            RTI->log("Error: File size of copied file does not match!");
            RTI->log("Error: Original size = " + QString::number(currentFilesize));
            RTI->log("Error: Copy size = " + QString::number(fileInfo.size()));
            RTI->setSevereErrors(true);
            return false;
        }
        else
        {
            RTI->log("Size of copied file matches with source " + QString::number(fileInfo.size()));
        }
    }

    if (RTI_CONFIG->isNetworkModeFTP())
    {
        // TODO
    }

    RTI->log("File transfer successful.");

    return true;
}


bool rdsNetwork::removeFile()
{
    RDS_RETONERR_LOG( queueDir.exists(currentFilename), "Error: File for deletion could not be found: " + currentFilename );

    if (!queueDir.remove(currentFilename))
    {
        RTI->log("Couldn't delete file " + currentFilename + ", retrying.");
        Sleep(DWORD(100));
        if (!queueDir.remove(currentFilename))
        {
            RTI->log("Error: Deleting file was not successful. File might be locked: " + currentFilename);
        }
    }

    return true;
}


void rdsNetwork::releaseFile()
{
    currentFilename="";
    currentProt="";
    currentFilesize=-1;
    currentTimeStamp="";
}


bool rdsNetwork::checkExistance(QString filename)
{
    int sepPos=filename.indexOf(RTI_SEPT_CHAR);

    if (sepPos==-1)
    {
        RTI->log("Error: File name does not contain protocol name: " + filename);
        return false;
    }

    QString fileProt=filename;
    fileProt.truncate(sepPos);

    if (RTI_CONFIG->isNetworkModeDrive())
    {
        QString fullFilename=RTI_CONFIG->netDriveBasepath + "/" + RTI_CONFIG->infoName + "/" + fileProt + "/" + filename;
        return QFile::exists(fullFilename);
    }

    if (RTI_CONFIG->isNetworkModeFTP())
    {
        // TODO
    }

    return false;
}


void rdsNetwork::copyLogFile()
{
    // Get the filename and path of the local log files from the log instance
    QString logName="rds.log";
    QString logPath=RTI->getLogInstance()->getLocalLogPath();

    // Delete the existing log file on the network drive inside the system folder
    // No error logging here because copying the log files is of low priority
    if (networkDrive.exists(logName))
    {
        if (!networkDrive.remove(logName))
        {
            RTI->log("ERROR: Can't remove existing log file on network drive.");
            return;
        }
    }

    // Stop the log engine (i.e. flush and close the log file)
    RTI->getLogInstance()->pauseLogfile();

    // Copy the log file into the network folder
    bool logCopySuccess=QFile::copy(logPath+logName, networkDrive.absoluteFilePath(logName));

    // Open the log file again
    RTI->getLogInstance()->resumeLogfile();

    if (!logCopySuccess)
    {
        RTI->log("WARNING: Copying the log file did not succeed!");
    }
}


void rdsNetwork::runReconnectCmd()
{
    // TODO: Change to rdsExecHelper to cover potential early return from QEventLoop

    // If a reconnect command has been defined, call it before the update
    if (RTI_CONFIG->netDriveReconnectCmd != "")
    {
        RTI->log("Calling network drive reconnect command.");

        QProcess *myProcess = new QProcess(0);
        myProcess->setReadChannel(QProcess::StandardOutput);

        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        QEventLoop q;
        connect(myProcess, SIGNAL(finished(int , QProcess::ExitStatus)), &q, SLOT(quit()));
        connect(&timeoutTimer, SIGNAL(timeout()), &q, SLOT(quit()));

        // If the command does not return within 1min, then kill it
        timeoutTimer.start(RDS_CONNECT_TIMEOUT);
        myProcess->start(RTI_CONFIG->netDriveReconnectCmd);
        q.exec();

        // Timer is still active, so the process seemed to termiante normally
        if (timeoutTimer.isActive())
        {
            timeoutTimer.stop();
        }

        if (myProcess->state()==QProcess::Running)
        {
            RTI->log("WARNING: The network drive reconnect command timed out!");
            myProcess->kill();
        }

        delete myProcess;
        myProcess=0;
    }
}



rdsCopyThread::rdsCopyThread()
    : QThread()
{
    success=false;
    sourceName="";
    destName="";
    finishedCopy=false;
}


void rdsCopyThread::run()
{
    finishedCopy=false;
    success=QFile::copy(sourceName,destName);
    finishedCopy=true;
    return;
}

