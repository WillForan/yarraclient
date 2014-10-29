#ifndef ORT_RECONTASK_H
#define ORT_RECONTASK_H

#include <QtCore>
#include <../Client/rds_raid.h>
#include <ort_network.h>
#include <ort_modelist.h>

class ortReconTask
{
public:
    ortReconTask();
    void setInstances(rdsRaid* raidinstance, ortNetwork* networkinstance);

    QString scanFile;
    QStringList adjustmentFiles;
    QString accNumber;
    QString emailNotifier;
    QString reconMode;
    QString systemName;
    QDateTime taskCreationTime;
    qint64 scanFileSize;
    QString patientName;
    QString scanProtocol;
    QString reconName;  
    int paramValue;

    bool highPriority;

    bool exportDataFiles(int fileID, ortModeEntry* mode);
    bool transferDataFiles();
    bool generateTaskFile();

    bool isSubmissionSuccessful();
    QString getErrorMessageUI();

    bool fileAlreadyExists;

protected:
    rdsRaid* raid;
    ortNetwork* network;

    QString errorMessageUI;
    bool reconTaskFailed;

};


inline bool ortReconTask::isSubmissionSuccessful()
{
    return (!reconTaskFailed);
}


inline QString ortReconTask::getErrorMessageUI()
{
    return errorMessageUI;
}


#endif // ORT_RECONTASK_H
