#ifndef YCTTWIXANONYMIZER_H
#define YCTTWIXANONYMIZER_H

#include <QtCore>


#define YCT_TWIXANONYMIZER_VER "0.1e"


class yctPatientInformation
{
public:

    yctPatientInformation()
    {
        name="";
        dateOfBirth="";
        mrn="";

        acc="";
        uuid="";
        taskid="";
        fillStr="";

        serialNumber="";
        patientWeight="";
        patientSex="";
    }

    // Information extracted from TWIX file
    QString name;
    QString dateOfBirth;
    QString mrn;

    // Externally delivered information
    QString acc;
    QString uuid;
    QString taskid;
    QString mode;    
    QString fillStr;

    // Additional information optionally extracted from TWIX file
    QString serialNumber;
    QString patientWeight;
    QString patientSex;
};


class yctTWIXAnonymizer
{
public:

    enum FileVersionType
    {
        UNKNOWN=0,
        VAVB,
        VDVE
    };

    enum AnalyzeResult
    {
        PROCESSING_ERROR                      = -1,
        NO_SENSITIVE_INFORMATION              =  0,
        SENSITIVE_INFORMATION_CLEARED         =  1,
        SENSITIVE_INFORMATION_FOLLOWS         =  2,
        SENSITIVE_INFORMATION_CLEARED_FOLLOWS =  3
    };

    enum ContentType
    {
        CONTENT_NAME                = 0,
        CONTENT_BIRTHDAY            = 1,
        CONTENT_ID                  = 2,
        CONTENT_VERSIONSTRING       = 3,
        CONTENT_REFPHYSICIAN        = 4,
        CONTENT_PATIENTREGISTRATION = 5
    };

    yctTWIXAnonymizer();

    bool processFile(QString twixFilename, QString phiPath, QString acc, QString taskid, QString uuid, QString mode, bool storePHI=true);
    bool processMeasurement(QFile* file);
    bool checkAndStorePatientData(QString twixFilename, QString phiPath);

    int analyzeLine(QByteArray* line);
    int clearLine(QByteArray* line);
    int analyzeFollowLine(QByteArray* line);
    bool clearPatientRegistrationEntry(QByteArray* line);

    FileVersionType fileVersion;
    int             expectedContent;

    QString errorReason;
    bool    debug;
    bool    testing;
    bool    dumpProtocol;
    bool    showOnlyInfo;

    bool    strictVersionChecking;
    void    setStrictVersionChecking(bool useStrictChecking);
    bool    checkVersionString(QString versionString);
    bool    versionStringSeen;
    bool    readAdditionalPatientInformation;

    QFile   dumpFile;

    yctPatientInformation patientInformation;

};


inline void yctTWIXAnonymizer::setStrictVersionChecking(bool useStrictChecking)
{
    strictVersionChecking=useStrictChecking;
}


#endif // YCTTWIXANONYMIZER_H
