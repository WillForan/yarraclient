#ifndef YCATASK_H
#define YCATASK_H

#include <QWidget>


class yctAPI;

class ycaTask
{    
public:
    enum TaskStatus
    {
        Invalid=0,
        Prepearing,
        Scheduled,
        Uploading,
        Processing,
        Downloading,
        Storage,
        Archvied,
        TranferError,
        ProcessingError
    };

    ycaTask();

    TaskStatus  status;

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
};

typedef QList<ycaTask*> ycaTaskList;


class ycaTaskHelper
{
public:

    ycaTaskHelper();
    void setCloudInstance(yctAPI* apiInstance);
    yctAPI* cloud;

    bool getScheduledTasks(ycaTaskList& taskList);
    bool getAllTasks(ycaTaskList& taskList, bool includeCurrent, bool includeArchive);
    bool checkScanfiles(QString taskID, ycaTask* task);    
    bool readPHIData(QString filepath, ycaTask* task);

    void clearTaskList(ycaTaskList& list);

};




#endif // YCATASK_H
