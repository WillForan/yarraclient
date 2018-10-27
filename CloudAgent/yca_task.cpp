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

    cost=-1;

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


QString ycaTask::getResult()
{
    QString str="";

    switch (result)
    {
    default:
    case trInProcess:
        str="Processing";
        break;
    case trSuccess:
        str="Sucess";
        break;
    case trAbortedTransfer:
        str="Aborted (Transfer)";
        break;
    case trAbortedProcessing:
        str="Aborted (Processing)";
        break;
    case trAbortedStorage:
        str="Aborted (Storage)";
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

    QString inPath=cloud->getCloudPath(YCT_CLOUDFOLDER_IN);

    QString phiPath=cloud->getCloudPath(YCT_CLOUDFOLDER_PHI);
    QDir phiDir(phiPath);
    if (!phiDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QFileInfoList fileList=phiDir.entryInfoList(QStringList("*.phi"),QDir::Files,QDir::Time);

    qDebug() << "Parsing folder...";

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

        QDir inTaskDir(inPath+"/"+uuid);
        if (inTaskDir.exists())
        {
            // Case has already been downloaded
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

    QString inPath=cloud->getCloudPath(YCT_CLOUDFOLDER_IN);

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
                taskList.append(task);
                continue;
            }

            QDir inTaskDir(inPath+"/"+uuid);
            if (inTaskDir.exists())
            {
                // Case has already been downloaded
                task->status=ycaTask::tsStorage;
            }
            else
            {
                // Case must be in the cloud or on its way in/out there
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

    if (!processingTasks.isEmpty())
    {
        cloud->getJobStatus(&processingTasks);
    }

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
        task->reconMode  =phiFile.value("PHI/MODE","").toString();

        task->result       =ycaTask::TaskResult(phiFile.value("STATUS/RESULT",ycaTask::trInProcess).toInt());
        task->retryDelay   =phiFile.value("STATUS/DELAY",QDateTime::currentDateTime()).toDateTime();
        task->uploadRetry  =phiFile.value("STATUS/RETRY_UPLOAD",0).toInt();
        task->downloadRetry=phiFile.value("STATUS/RETRY_DOWNLOAD",0).toInt();
        task->storageRetry =phiFile.value("STATUS/RETRY_STORAGE",0).toInt();
    }

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


bool ycaTaskHelper::saveCostsToPHI(ycaTaskList& taskList)
{
    for (int i=0; i<taskList.count(); i++)
    {
        ycaTask* task=taskList.at(i);
        QString  filepath=cloud->getCloudPath(YCT_CLOUDFOLDER_PHI)+"/"+task->uuid+".phi";

        if (!QFile::exists(filepath))
        {
            qDebug() << "Error: PHI file not found.";
            continue;
        }

        QSettings phiFile(filepath, QSettings::IniFormat);

        if (phiFile.value("PHI/UUID","").toString()!=task->uuid)
        {
            // UUID is missing. PHI file seems invalid or unable to read.
            continue;
        }

        phiFile.setValue("STATS/COSTS",task->cost);
    }

    return true;
}



void ycaTaskHelper::getTasksForDownloadArchive(ycaTaskList& taskList, ycaTaskList& downloadList, ycaTaskList& archiveList)
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


bool ycaTaskHelper::archiveTasks(ycaTaskList& archiveList)
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

        // If the job has not been flagged as complete or aborted,
        // don't move it
        if (currentTask->result==ycaTask::trInProcess)
        {
            continue;
        }

        //qInfo() << "Processing file: " << phiPath+"/"+currentTask->phiFilename;
        //qInfo() << "Moving to: " << archivePath+"/"+currentTask->phiFilename;

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


bool ycaTaskHelper::removeIncompleteDownloads()
{
    QString inPath=cloud->getCloudPath(YCT_CLOUDFOLDER_IN);
    QDir inDir(inPath);
    if (!inDir.exists())
    {
        // TODO: Error reporting
        return false;
    }

    QFileInfoList dirList=inDir.entryInfoList(QStringList("*"),QDir::Dirs|QDir::NoDotAndDotDot,QDir::Time);

    for (int i=0; i<dirList.count(); i++)
    {
        //qInfo() << dirList.at(i).filePath()+"/"+YCT_INCOMPLETE_FILE;

        if (QFile::exists(dirList.at(i).filePath()+"/"+YCT_INCOMPLETE_FILE))
        {
            // INCOMPLETE file found, so folder is from incomplete download.
            // Remove the folder and download again.
            QDir incompleteDir(dirList.at(i).filePath());
            if (!incompleteDir.removeRecursively())
            {
                // TODO: Error handling!
            }
        }
    }

    return true;
}


bool ycaTaskHelper::storeTasks(ycaTaskList& archiveList, QObject* notificationWidget)
{
    QString inPath=cloud->getCloudPath(YCT_CLOUDFOLDER_IN);
    QDir inDir(inPath);
    if (!inDir.exists())
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

    QFileInfoList dirList=inDir.entryInfoList(QStringList("*"),QDir::Dirs|QDir::NoDotAndDotDot,QDir::Time);

    if ((notificationWidget!=0) && (!dirList.isEmpty()))
    {
        QMetaObject::invokeMethod(notificationWidget, "showIndicator", Qt::QueuedConnection);
    }

    for (int i=0; i<dirList.count(); i++)
    {
        QString incompleteFile=dirList.at(i).filePath()+"/"+YCT_INCOMPLETE_FILE;

        if (QFile::exists(incompleteFile))
        {
            qDebug() << "INCOMPLETE file found, folder is from incomplete download: " << incompleteFile;

            // INCOMPLETE file found, so folder is from incomplete download.
            continue;
        }

        QString phiFile=phiDir.absoluteFilePath(dirList.at(i).fileName()+".phi");
        if (!QFile::exists(phiFile))
        {
            qDebug() << "File with PHI information not found. Can't store task: " << phiFile;

            // Corresponding file with PHI information not found. Can't store task.
            // TODO: Error handling
            continue;
        }

        ycaTask* currentTask=new ycaTask;
        currentTask->uuid=dirList.at(i).fileName();

        if (!readPHIData(phiFile, currentTask))
        {            
            qDebug() << "Error reading PHI file " << phiFile;

            delete currentTask;
            currentTask=0;
            continue;
        }

        if (!cloud->insertPHI(dirList.at(i).filePath(),currentTask))
        {
            // TODO: Error reporting
            delete currentTask;
            currentTask=0;
            continue;
        }

        if (!cloud->pushToDestinations(dirList.at(i).filePath(),currentTask))
        {
            // TODO: Error handling
            delete currentTask;
            currentTask=0;

            // If an error occurred during transfer to one of the destinations,
            // don't delete the folder and try again next time
            continue;
        }

        // Folder cleanup
        QDir removeDir(dirList.at(i).filePath());
        if (!removeDir.removeRecursively())
        {
            qDebug() << "Error removing files from IN folder";
            // TODO: Error reporting!
        }

        currentTask->status=ycaTask::tsStorage;
        currentTask->result=ycaTask::trSuccess;
        archiveList.append(currentTask);

        //qInfo() << "Name:";        
        //qInfo() << dirList.at(i).fileName();
    }

    return true;
}

