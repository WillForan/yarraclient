#include "yca_task.h"
#include "yca_task.h"

#include "../CloudTools/yct_api.h"
#include "../CloudTools/yct_common.h"


ycaTask::ycaTask()
{
    status=Invalid;

    patientName="";
    uuid="";
    shortcode="";
    taskFilename="";
    phiFilename="";

    twixFilenames.clear();
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
    // TODO: Insert mutex

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


bool ycaTaskHelper::getRunningTasks(ycaTaskList& taskList)
{
    clearTaskList(taskList);

    //TODO

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
        taskList.append(task);
    }
}


void ycaTaskHelper::clearTaskList(ycaTaskList& list)
{
    while (!list.isEmpty())
    {
          delete list.takeFirst();
    }
}


bool ycaTaskHelper::getAllTasks(ycaTaskList& taskList, bool includeCurrent, bool includeArchive)
{
    // TODO: Insert mutex

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
                task->status=ycaTask::Prepearing;
                taskList.append(task);
                continue;
            }

            if (!readPHIData(fileList.at(i).absoluteFilePath(),task))
            {
                // TODO: Error handling
            }

            if (outDir.exists(uuid+".task"))
            {
                task->status=ycaTask::Scheduled;
            }
            else
            {
                task->status=ycaTask::Processing;
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
            task->status=ycaTask::Archvied;
            if (!readPHIData(fileList.at(i).absoluteFilePath(),task))
            {
                // TODO: Error handling
            }

            taskList.append(task);
        }
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

    // TODO: Error checking

    return true;
}
