#ifndef RDS_ANONYMIZEVB17_H
#define RDS_ANONYMIZEVB17_H

#include <QtCore>

class rdsAnonymizeVB17
{
public:
    rdsAnonymizeVB17();

    static bool anonymize(QString filename);
    bool perform(QString filename);


    enum TWIXFileTypes
    {
        TWIX_INVALID = 0,
        TWIX_VB17    = 1
    };

    enum AnalyzeResult
    {
        NO_SENSITIVE_INFORMATION      = 0,
        SENSITIVE_INFORMATION_CLEARED = 1,
        SENSITIVE_INFORMATION_FOLLOWS = 2
    };

    enum ContentType
    {
        CONTENT_NAME     = 0,
        CONTENT_BIRTHDAY = 1,
        CONTENT_ID       = 2
    };

    bool anonymizeVB17(QString filename);
    int analyzeLine(QByteArray* line);
    int analyzeFollowLine(QByteArray* line);
    int clearLine(QByteArray* line);

    QString selectedDirectory;
    bool isCanceled;
    int expectedContent;
    bool addedLogMessage;



};

#endif // RDS_ANONYMIZEVB17_H

