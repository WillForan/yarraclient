#ifndef YCATASK_H
#define YCATASK_H

#include <QWidget>
#include <QDateTime>


class yctAPI;

class ycaTask
{    
public:

    enum TaskStatus
    {
        tsInvalid=-1,
        tsPreparing=0,     // SAC/ORT are currently preparing the job
        tsScheduled,       // Job is scheduled for upload to cloud
        tsProcessing,      // Job is currently being processed, i.e. residing in the cloud (or upoladed/downloaded)
        tsUploading,       // Job is currently being uploaded (only relevant is called outside of worker thread)
        tsRunning,         // Job is running/scheduled in the cloud
        tsReady,           // Job is finished and awaiting download
        tsDownloading,     // Job is currently being uploaded (only relevant is called outside of worker thread)
        tsStorage,         // Results are being stored
        tsArchived,        // Job resides in the archive
        tsErrorTransfer,   // Error occured on client side
        tsErrorProcessing, // Error occured on the cloud side
        tsErrorStorage     // Error occured on client side
    };

    enum TaskResult
    {
        trInProcess=-1,
        trSuccess=0,
        trAbortedTransfer,
        trAbortedProcessing,
        trAbortedStorage
    };

    enum WorkerProcess
    {
        wpIdle=0,
        wpUpload,
        wpDownload,
        wpStorage
    };

    ycaTask();

    TaskStatus  status;
    TaskResult  result;
    QString     getStatus();

    QString     patientName;
    QString     uuid;
    QString     shortcode;
    QString     reconMode;

    QString     mrn;
    QString     dob;
    QString     acc;
    QString     taskID;

    QString     taskFilename;
    QString     phiFilename;
    QStringList twixFilenames;

    QDateTime   retryDelay;
    int         uploadRetry;
    int         downloadRetry;
    int         storageRetry;
};

typedef QList<ycaTask*> ycaTaskList;


class ycaTaskHelper
{
public:

    ycaTaskHelper();
    void setCloudInstance(yctAPI* apiInstance);
    yctAPI* cloud;

    bool getScheduledTasks(ycaTaskList& taskList);
    bool getProcessingTasks(ycaTaskList& taskList);
    bool getAllTasks(ycaTaskList& taskList, bool includeCurrent, bool includeArchive, QString workerJobID="", ycaTask::WorkerProcess workerOperation=ycaTask::wpIdle);
    bool checkScanfiles(QString taskID, ycaTask* task);    
    bool readPHIData(QString filepath, ycaTask* task);
    bool saveResultToPHI(QString filepath, ycaTask::TaskResult result);

    void getJobsForDownloadArchive(ycaTaskList& taskList, ycaTaskList& downloadList, ycaTaskList& archiveList);
    bool archiveJobs(ycaTaskList& archiveList);
    void clearTaskList(ycaTaskList& list);

};




#endif // YCATASK_H
