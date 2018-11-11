#ifndef ORT_RECONTASK_H
#define ORT_RECONTASK_H

#include <QtCore>
#include <../Client/rds_raid.h>
#include <ort_network.h>
#include <ort_modelist.h>


class ortConfiguration;

class ortReconTask
{
public:
    ortReconTask();
    void setInstances(rdsRaid* raidinstance, ortNetwork* networkinstance, ortConfiguration* configinstance);

    QString     scanFile;
    QStringList adjustmentFiles;
    QString     accNumber;
    QString     emailNotifier;
    QString     reconMode;
    QString     systemName;
    QDateTime   taskCreationTime;
    QString     raidCreationTime;
    qint64      scanFileSize;
    QString     patientName;
    QString     scanProtocol;
    QString     reconName;
    int         paramValue;
    QString     requiredServerType;
    QString     selectedServer;

    bool        highPriority;

    QString     uuid;
    bool        cloudReconstruction;

    bool exportDataFiles(int fileID, ortModeEntry* mode);
    bool transferDataFiles();
    bool anonymizeFiles();
    bool generateTaskFile();

    bool isSubmissionSuccessful();
    QString getErrorMessageUI();

    bool fileAlreadyExists;

    QString cloudOUTpath;
    QString cloudPHIpath;
    void setCloudPaths(QString outPath, QString phiPath);

protected:
    rdsRaid* raid;
    ortNetwork* network;
    ortConfiguration* config;

    QString errorMessageUI;
    bool reconTaskFailed;

    bool removeTaskFiles();

};


inline bool ortReconTask::isSubmissionSuccessful()
{
    return (!reconTaskFailed);
}


inline QString ortReconTask::getErrorMessageUI()
{
    return errorMessageUI;
}


inline void ortReconTask::setCloudPaths(QString outPath, QString phiPath)
{
    cloudOUTpath=outPath;
    cloudPHIpath=phiPath;
}


#endif // ORT_RECONTASK_H

