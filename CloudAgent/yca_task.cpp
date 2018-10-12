#include "yca_task.h"
#include "yca_task.h"

#include "../CloudTools/yct_api.h"
#include "../CloudTools/yct_common.h"


ycaTask::ycaTask()
{
    status=tsInvalid;
    result=trInProcess;

    patientName="";
    uuid="";
    shortcode="";
    taskFilename="";
    phiFilename="";

    retryDelay=QDateTime::currentDateTime();
    uploadRetry=0;
    downloadRetry=0;
    storageRetry=0;

    twixFilenames.clear();
}


QString ycaTask::getStatus()
{
    QString str="";

    switch (status)
    {
    default:
    case tsInvalid:
        str="Invalid";
        break;
    case tsPreparing:
        str="Preparing";
        break;
    case tsScheduled:
        str="Scheduled";
        break;
    case tsProcessing:
        str="Processing";
        break;
    case tsUploading:
        str="Uploading";
        break;
    case tsRunning:
        str="Running";
        break;
    case tsReady:
        str="Ready";
        break;
    case tsDownloading:
        str="Downloading";
        break;
    case tsStorage:
        str="Storage";
        break;
    case tsArchived:
        str="Completed";
        break;
    case tsErrorTransfer:
        str="Transfer Error";
        break;
    case tsErrorProcessing:
        str="Processing Error";
        break;
    }

    return str;
}


ycaTaskHelper::ycaTaskHelper()
{
    cloud=0;
}


void ycaTaskHelper::setCloudInstance(yctAPI* apiInstance)
{
    cloud=apiInstance;
}


bool ycaTaskHelper::getScheduledTasks(ycaTaskList& taskList)
{
    clearTaskList(taskList);

    QString outPath=cloud->getCloudPath(YCT_CLOUDFOLDER_OUT);
    QDir outDir(outPath);
    if (!outDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QString phiPath=cloud->getCloudPath(YCT_CLOUDFOLDER_PHI);
    QDir phiDir(phiPath);
    if (!phiDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QFileInfoList fileList=outDir.entryInfoList(QStringList("*.task"),QDir::Files,QDir::Time);

    //qInfo() << "Parsing folder...";

    for (int i=0; i<fileList.count(); i++)
    {
        QString uuid=fileList.at(i).baseName();

        if (outDir.exists(uuid+".lock"))
        {
            // File is currently being written. So ignore it for now.
            continue;
        }

        if (!phiDir.exists(uuid+".phi"))
        {
            // PHI file is missing! Something must be wrong.
            // TODO: Error reporting
            continue;
        }

        ycaTask* task=new ycaTask();
        task->uuid=uuid;
        task->taskFilename=uuid+".task";

        if (!checkScanfiles(uuid,task))
        {
            // Scan files are missing! Something must be wrong.
            // TODO: Error reporting

            delete task;
            task=0;
            continue;
        }

        taskList.append(task);
    }

    return true;
}


bool ycaTaskHelper::getProcessingTasks(ycaTaskList& taskList)
{
    clearTaskList(taskList);

    QString outPath=cloud->getCloudPath(YCT_CLOUDFOLDER_OUT);
    QDir outDir(outPath);
    if (!outDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QString phiPath=cloud->getCloudPath(YCT_CLOUDFOLDER_PHI);
    QDir phiDir(phiPath);
    if (!phiDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QFileInfoList fileList=phiDir.entryInfoList(QStringList("*.phi"),QDir::Files,QDir::Time);

    //qInfo() << "Parsing folder...";

    for (int i=0; i<fileList.count(); i++)
    {
        QString uuid=fileList.at(i).baseName();

        if (outDir.exists(uuid+".lock"))
        {
            // File is currently being written. So ignore it for now.
            continue;
        }

        if (outDir.exists(uuid+".task"))
        {
            // File has not been uploaded yet
            continue;
        }

        ycaTask* task=new ycaTask();
        task->uuid=uuid;
        task->taskFilename=uuid+".task";
        task->phiFilename=uuid+".phi";
        task->status=ycaTask::tsProcessing;
        taskList.append(task);        
    }

    return true;
}


void ycaTaskHelper::clearTaskList(ycaTaskList& list)
{
    while (!list.isEmpty())
    {
          delete list.takeFirst();
    }
}


bool ycaTaskHelper::getAllTasks(ycaTaskList& taskList, bool includeCurrent, bool includeArchive, QString workerJobID, ycaTask::WorkerProcess workerOperation)
{
    clearTaskList(taskList);

    QString phiPath=cloud->getCloudPath(YCT_CLOUDFOLDER_PHI);
    QDir phiDir(phiPath);
    if (!phiDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QString archivePath=cloud->getCloudPath(YCT_CLOUDFOLDER_ARCHIVE);
    QDir archiveDir(archivePath);
    if (!archiveDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QString outPath=cloud->getCloudPath(YCT_CLOUDFOLDER_OUT);
    QDir outDir(outPath);
    if (!outDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    if (includeCurrent)
    {
        QFileInfoList fileList=phiDir.entryInfoList(QStringList("*.phi"),QDir::Files,QDir::Time);

        for (int i=0; i<fileList.count(); i++)
        {
            QString uuid=fileList.at(i).baseName();
            ycaTask* task=new ycaTask();
            task->uuid=uuid;

            if (outDir.exists(uuid+".lock"))
            {
                // File is currently being written. So ignore it for now.
                task->status=ycaTask::tsPreparing;
                taskList.append(task);
                continue;
            }

            if (!readPHIData(fileList.at(i).absoluteFilePath(),task))
            {
                // TODO: Error handling
            }

            if (outDir.exists(uuid+".task"))
            {
                task->status=ycaTask::tsScheduled;
            }
            else
            {
                task->status=ycaTask::tsProcessing;
            }

            taskList.append(task);
        }
    }

    if (includeArchive)
    {
        QFileInfoList fileList=archiveDir.entryInfoList(QStringList("*.phi"),QDir::Files,QDir::Time);

        for (int i=0; i<fileList.count(); i++)
        {
            QString uuid=fileList.at(i).baseName();
            ycaTask* task=new ycaTask();
            task->uuid=uuid;
            task->status=ycaTask::tsArchived;
            if (!readPHIData(fileList.at(i).absoluteFilePath(),task))
            {
                // TODO: Error handling
            }

            taskList.append(task);
        }
    }

    // Now fetch the status of the jobs that are living in the cloud currently
    ycaTaskList processingTasks;
    for (int i=0; i<taskList.count(); i++)
    {
        if (taskList.at(i)->status==ycaTask::tsProcessing)
        {
            if (workerJobID==taskList.at(i)->uuid)
            {
                // If the worker thread is currently uploading/downloading the job
                switch (workerOperation)
                {
                case ycaTask::wpUpload:
                    taskList.at(i)->status=ycaTask::tsUploading;
                    break;
                case ycaTask::wpDownload:
                    taskList.at(i)->status=ycaTask::tsDownloading;
                    break;
                case ycaTask::wpStorage:
                    taskList.at(i)->status=ycaTask::tsStorage;
                    break;
                default:
                    break;
                };
            }
            else
            {
                processingTasks.append(taskList.at(i));
            }
        }
    }
    cloud->getJobStatus(&processingTasks);

    return true;
}


bool ycaTaskHelper::checkScanfiles(QString taskID, ycaTask* task)
{
    QSettings taskFile(cloud->getCloudPath(YCT_CLOUDFOLDER_OUT)+"/"+taskID+".task", QSettings::IniFormat);

    if (taskFile.value("Task/UUID","").toString() != task->uuid)
    {
        // UUID does not match. Something is wrong
        return false;
    }

    task->twixFilenames.clear();

    QStringList tempStrList=taskFile.value("Task/ScanFile","").toStringList();
    QString filename=tempStrList.join(",");
    task->twixFilenames.append(filename);

    int  i=0;
    bool stop=false;

    do
    {
        tempStrList=taskFile.value("AdjustmentFiles/"+QString(i),"").toStringList();
        filename=tempStrList.join(",");

        if (filename.isEmpty())
        {
            stop=true;
        }
        else
        {
            task->twixFilenames.append(filename);
        }

        i++;
        if (i>99)
        {
            stop=true;
        }
    }
    while (!stop);

    QString outPath=cloud->getCloudPath(YCT_CLOUDFOLDER_OUT);
    QDir outDir(outPath);

    bool fileMissing=false;
    for (int j=0; j<task->twixFilenames.count(); j++)
    {
        //qInfo() << task->twixFilenames.at(j);

        if (!outDir.exists(task->twixFilenames.at(j)))
        {
            fileMissing=true;
        }
    }

    if (fileMissing)
    {
        return false;
    }

    task->reconMode=taskFile.value("Task/ReconMode","").toString();

    return true;
}


bool ycaTaskHelper::readPHIData(QString filepath, ycaTask* task)
{
    QSettings phiFile(filepath, QSettings::IniFormat);

    if (phiFile.value("PHI/UUID","").toString() != task->uuid)
    {
        // UUID does not match. Something is wrong
        return false;
    }

    task->patientName=phiFile.value("PHI/NAME","").toString();
    task->mrn        =phiFile.value("PHI/MRN","").toString();
    task->dob        =phiFile.value("PHI/DOB","").toString();
    task->acc        =phiFile.value("PHI/ACC","").toString();
    task->taskID     =phiFile.value("PHI/TASKID","").toString();

    task->result       =ycaTask::TaskResult(phiFile.value("STATUS/RESULT",ycaTask::trInProcess).toInt());
    task->retryDelay   =phiFile.value("STATUS/DELAY",QDateTime::currentDateTime()).toDateTime();
    task->uploadRetry  =phiFile.value("STATUS/RETRY_UPLOAD",0).toInt();
    task->downloadRetry=phiFile.value("STATUS/RETRY_DOWNLOAD",0).toInt();
    task->storageRetry =phiFile.value("STATUS/RETRY_STORAGE",0).toInt();

    // TODO: Error checking

    return true;
}


bool ycaTaskHelper::saveResultToPHI(QString filepath, ycaTask::TaskResult result)
{
    QSettings phiFile(filepath, QSettings::IniFormat);

    if (phiFile.value("PHI/UUID","").toString().isEmpty())
    {
        // UUID is missing. PHI file seems invalid.
        return false;
    }

    phiFile.setValue("STATUS/RESULT",result);

    return true;
}


void ycaTaskHelper::getJobsForDownloadArchive(ycaTaskList& taskList, ycaTaskList& downloadList, ycaTaskList& archiveList)
{
    clearTaskList(downloadList);
    clearTaskList(archiveList);

    for (int i=0; i<taskList.count(); i++)
    {
        if (taskList.at(i)->status==ycaTask::tsReady)
        {
            downloadList.append(taskList.at(i));
        }
        else
        {
            if (taskList.at(i)->status==ycaTask::tsErrorProcessing)
            {
                archiveList.append(taskList.at(i));
            }
        }
    }
}


bool ycaTaskHelper::archiveJobs(ycaTaskList& archiveList)
{
    QString phiPath=cloud->getCloudPath(YCT_CLOUDFOLDER_PHI);
    QDir phiDir(phiPath);
    if (!phiDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QString archivePath=cloud->getCloudPath(YCT_CLOUDFOLDER_ARCHIVE);
    QDir archiveDir(archivePath);
    if (!archiveDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    while (!archiveList.isEmpty())
    {
        ycaTask* currentTask=archiveList.takeFirst();

        qInfo() << "Processing file: " << phiPath+"/"+currentTask->phiFilename;
        qInfo() << "Moving to: " << archivePath+"/"+currentTask->phiFilename;

        // Move the file into the archive folder
        if (!phiDir.rename(phiPath+"/"+currentTask->phiFilename, archivePath+"/"+currentTask->phiFilename))
        {
            // TODO: Error handling
            return false;
        }

        ycaTask::TaskResult result=ycaTask::trInProcess;

        switch (currentTask->status)
        {
        case ycaTask::tsStorage:
            result=ycaTask::trSuccess;
            break;
        case ycaTask::tsErrorProcessing:
            result=ycaTask::trAbortedProcessing;
            break;
        case ycaTask::tsErrorTransfer:
            result=ycaTask::trAbortedTransfer;
            break;
        case ycaTask::tsErrorStorage:
            result=ycaTask::trAbortedStorage;
            break;
        default:
            break;
        }

        // Now, add the result status to the moved PHI file
        saveResultToPHI(archivePath+"/"+currentTask->phiFilename, result);

        currentTask->status=ycaTask::tsArchived;
    }

    return true;
}

