#ifndef YCTTWIXANONYMIZER_H
#define YCTTWIXANONYMIZER_H

#include <QtCore>


#define YCT_TWIXANONYMIZER_VER "0.1b"


class yctPatientInformation
{
public:

    yctPatientInformation()
    {
        name="";
        dateOfBirth="";
        mrn="";
        acc="";
    }

    QString name;
    QString dateOfBirth;
    QString mrn;
    QString acc;
    QString uuid;
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
        PROCESSING_ERROR              = -1,
        NO_SENSITIVE_INFORMATION      =  0,
        SENSITIVE_INFORMATION_CLEARED =  1,
        SENSITIVE_INFORMATION_FOLLOWS =  2
    };

    enum ContentType
    {
        CONTENT_NAME     = 0,
        CONTENT_BIRTHDAY = 1,
        CONTENT_ID       = 2
    };

    yctTWIXAnonymizer();

    bool processFile(QString twixFilename, QString taskFilename, QString phiPath);
    bool processMeasurement(QFile* file);
    bool cleanTaskFile(QString taskFilename);
    bool checkAndStorePatientData(QString taskFilename, QString phiPath);

    int analyzeLine(QByteArray* line);
    int clearLine(QByteArray* line);
    int analyzeFollowLine(QByteArray* line);

    FileVersionType fileVersion;
    int             expectedContent;

    QString errorReason;
    bool    debug;
    bool    testing;
    bool    dumpProtocol;

    QFile   dumpFile;

    yctPatientInformation patientInformation;

};

#endif // YCTTWIXANONYMIZER_H
