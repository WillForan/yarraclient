#ifndef YCTAPI_H
#define YCTAPI_H

#include <QString>


class yctConfiguration;
class ortModeList;

class yctAPI
{
public:

    yctAPI();
    void    setConfiguration(yctConfiguration* configuration);
    int     readModeList(ortModeList* modeList);
    void    launchCloudAgent(QString params="");
    bool    createCloudFolders();
    bool    validateUser();
    QString getCloudPath(QString folder);
    QString createUUID();

    QString errorReason;

protected:

    yctConfiguration* config;

};


#endif // YCTAPI_H
