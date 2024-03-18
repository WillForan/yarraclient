#include "ort_recontask.h"
#include "ort_global.h"
#include "ort_configuration.h"

#include "../CloudTools/yct_prepare/yct_twix_anonymizer.h"


ortReconTask::ortReconTask()
{
    raid=0;
    config=0;
    network=0;

    scanFile="";
    adjustmentFiles.clear();
    accNumber="";
    emailNotifier="";
    reconMode="";
    systemName="";
    taskCreationTime=QDateTime::currentDateTime();
    raidCreationTime=taskCreationTime.toString("dd/MM/yy")+"  "+taskCreationTime.toString("HH:mm:ss");
    patientName="";
    scanProtocol="";
    reconName="";
    paramValue=0;
    requiredServerType="";
    selectedServer="";

    errorMessageUI="";
    reconTaskFailed=false;
    fileAlreadyExists=false;

    highPriority=false;

    uuid="";
    cloudReconstruction=false;
    cloudOUTpath="";
    cloudPHIpath="";
}


void ortReconTask::setInstances(rdsRaid* raidinstance, ortNetwork* networkinstance, ortConfiguration* configinstance)
{
    raid=raidinstance;
    network=networkinstance;
    config=configinstance;
}


bool ortReconTask::exportDataFiles(int fileID, ortModeEntry* mode)
{
    if ((raid==0) || (network==0) || (config==0))
    {
        return false;
    }

    adjustmentFiles.clear();

    QString modeSuffix="";
    if (mode->paramDescription!="")
    {
        modeSuffix=QString::number(paramValue);
    }

    QString cloudUUID="";
    if (cloudReconstruction)
    {
        cloudUUID=uuid;        

        // Temporarily redirect the export path to the cloud folder
        raid->queueDir.cd(cloudOUTpath);
    }

    if (!raid->saveSingleFile(fileID, mode->requiresAdjScans, mode->idName, scanFile, adjustmentFiles, modeSuffix,cloudUUID))
    {
        reconTaskFailed=true;
        RTI->log("ERROR: Export of raid data failed.");

        if (raid->ortMissingDiskspace)
        {
            errorMessageUI="Insufficient disk space available on local drive to export data.";
        }
        else
        {
            errorMessageUI="Local export of scan data failed.";
        }
        return false;
    }

    if (cloudReconstruction)
    {
        // Reset the export path to the regular queue folder
        raid->queueDir.cd(RTI->getAppPath() + "/" + ORT_DIR_QUEUE);
    }

    return true;
}


bool ortReconTask::transferDataFiles()
{
    if ((raid==0) || (network==0) || (config==0))
    {
        return false;
    }

    QFileInfo fileInfo(network->queueDir, scanFile);
    if (!fileInfo.exists())
    {
        reconTaskFailed=true;
        RTI->log(QString("ERROR: Could not find previously exported file ")+scanFile+" in "+ network->queueDir.absolutePath());
        errorMessageUI="Exported files could not be found.";
        return false;
    }

    scanFileSize=fileInfo.size();
    if (scanFileSize==0)
    {
        reconTaskFailed=true;
        RTI->log("ERROR: Size of scan if file is not valid.");
        errorMessageUI="Size of exported files invalid.";
        return false;
    }

    // Check if file any of the files already exist in queue dir. If so, inform user.
    if (network->serverTaskDir.exists(scanFile))
    {
        fileAlreadyExists=true;
        RTI->log("NOTE: File already exists on server.");
        return false;
    }

    if (!network->transferQueueFiles())
    {
        reconTaskFailed=true;
        RTI->log("ERROR: Transfer of files to server failed.");
        errorMessageUI="Transfer to Yarra server failed.";
        return false;
    }

    return true;
}


bool ortReconTask::generateTaskFile()
{
    if ((raid==0) || (network==0) || (config==0))
    {
        return false;
    }

    QString taskFilename=scanFile;
    taskFilename.truncate(taskFilename.indexOf("."));
    QString lockFilename=taskFilename;
    lockFilename+=ORT_LOCK_EXTENSION;
    taskFilename+=ORT_TASK_EXTENSION;
    if (highPriority)
    {
        taskFilename+=ORT_TASK_EXTENSION_PRIO;
    }

    taskCreationTime=QDateTime::currentDateTime();

    QString lockFilePath=network->serverTaskDir.filePath(lockFilename);
    if (cloudReconstruction)
    {
        lockFilePath=cloudOUTpath+"/"+lockFilename;
    }
    QLockFile lockFile(lockFilePath);

    if (!lockFile.isLocked())
    {
        // Create write lock for the task file (to avoid that the server might
        // access the file is not entirely written)
        lockFile.lock();

        // Scoping for the lock file
        {
            // Create the task file
            QString taskFilePath=network->serverTaskDir.filePath(taskFilename);
            if (cloudReconstruction)
            {
                taskFilePath=cloudOUTpath+"/"+taskFilename;
            } else if (QString(network->metaObject()->className()) == QString("ortNetworkSftp")) {
                // If we are using the sftp uploader we need to put the task somewhere locally and then upload it.
                taskFilePath=network->queueDir.filePath(taskFilename);
            }
            QSettings taskFile(taskFilePath, QSettings::IniFormat);
            // Write the entries
            taskFile.setValue("Task/ReconMode", reconMode);
            taskFile.setValue("Task/EMailNotification", emailNotifier);
            taskFile.setValue("Task/ScanFile", scanFile);
            taskFile.setValue("Task/AdjustmentFilesCount", adjustmentFiles.count());
            taskFile.setValue("Task/ScanProtocol", scanProtocol);
            taskFile.setValue("Task/ReconName", reconName);
            taskFile.setValue("Task/ParamValue", paramValue);
            taskFile.setValue("Task/RequiredServerType", requiredServerType);

            if (!cloudReconstruction)
            {
                taskFile.setValue("Task/ACC", accNumber);
                taskFile.setValue("Task/PatientName", patientName);
            }
            else
            {
                taskFile.setValue("Task/UUID", uuid);
            }

            taskFile.setValue("Task/CreationTimeRAID", raidCreationTime);
            taskFile.setValue("Task/CreationTimeTask", taskCreationTime.toString("dd/MM/yy")+"  "+taskCreationTime.toString("HH:mm:ss"));

            taskFile.setValue("Information/ScanFileSize",    scanFileSize);
            taskFile.setValue("Information/TaskDate",        taskCreationTime.date().toString(Qt::ISODate));
            taskFile.setValue("Information/TaskTime",        taskCreationTime.time().toString(Qt::ISODate));
            taskFile.setValue("Information/SelectedServer",  selectedServer);
            taskFile.setValue("Information/SerialNumber",    config->infoSerialNumber);
            taskFile.setValue("Information/SystemName",      systemName);
            taskFile.setValue("Information/SystemVendor",    "Siemens");
            taskFile.setValue("Information/SystemType",      config->infoScannerType);
            taskFile.setValue("Information/SoftwareVersion", config->infoSoftwareVersion);
            taskFile.setValue("Information/SystemVersion",   RTI->getSyngoMRVersionString());

            // Write the list of adjustment files as strings
            for (int i=0; i<adjustmentFiles.count(); i++)
            {
                taskFile.setValue("AdjustmentFiles/"+QString(i), adjustmentFiles.at(i));
            }

            // Flush the entries into the file
            taskFile.sync();

            RTI->processEvents();
        }

        // Free the lock file
        lockFile.unlock();


        if (QString(network->metaObject()->className()) == QString("ortNetworkSftp")) {
            // If we are using the sftp uploader we need to put the task somewhere locally and then upload it.
            network->currentFilename = taskFilename;
            network->copyFile();
        }
        RTI->log("Generated task file. Done with submission.");
    }
    else
    {
        reconTaskFailed=true;
        RTI->log("ERROR: Can't lock task file for submission.");
        errorMessageUI="Creation of task file failed.";

        removeTaskFiles();

        return false;
    }

    return true;
}


bool ortReconTask::anonymizeFiles()
{
    bool success=true;
    QString taskID=raid->ortTaskID;

    {
        yctTWIXAnonymizer twixAnonymizer;

        if (!twixAnonymizer.processFile(cloudOUTpath+"/"+scanFile, cloudPHIpath,
                                        accNumber, taskID, uuid, reconMode))
        {
            RTI->log("Error while anonymizing TWIX file "+scanFile);
            errorMessageUI="Error while anonymizing raw-data file.";
            success=false;
        }
    }

    for (int i=0; i<adjustmentFiles.count(); i++)
    {
        yctTWIXAnonymizer twixAnonymizer;

        if (!twixAnonymizer.processFile(cloudOUTpath+"/"+adjustmentFiles.at(i), cloudPHIpath,
                                        accNumber, taskID, uuid, reconMode, false))
        {
            RTI->log("Error while anonymizing adjustment file "+adjustmentFiles.at(i));
            errorMessageUI="Error while anonymizing adjustment files.";
            success=false;
        }
    }

    if (!success)
    {
        removeTaskFiles();
    }

    return success;
}


bool ortReconTask::removeTaskFiles()
{
    bool success=true;

    // Remove all files
    QDir outDir(cloudOUTpath);
    QFileInfoList fileList=outDir.entryInfoList(QStringList(uuid+"*.*"),QDir::Files,QDir::Time);

    for (int i=0; i<fileList.count(); i++)
    {
        RTI->log("Removing scan file "+fileList.at(i).fileName());
        if (!outDir.remove(fileList.at(i).fileName()))
        {
            RTI->log("Error removing file "+fileList.at(i).fileName());
            success=false;
        }
    }

    if (QFile::exists(cloudPHIpath+"/"+uuid+".phi"))
    {
        RTI->log("Removing phi file");
        QString phiFilename=cloudPHIpath+"/"+uuid+".phi";
        if (!QFile::remove(phiFilename))
        {
            RTI->log("Error removing phi file "+phiFilename);
            success=false;
        }
    }

    return success;
}


