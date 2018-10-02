#ifndef YCTAPI_H
#define YCTAPI_H

#include <QString>


class yctConfiguration;
class ortModeList;
class ycaTask;

class yctAPI
{
public:

    yctAPI();
    void    setConfiguration(yctConfiguration* configuration);

#ifdef YARRA_APP_SAC
    int     readModeList(ortModeList* modeList);
    void    launchCloudAgent(QString params="");
#endif
    bool    createCloudFolders();
    bool    validateUser();
    QString getCloudPath(QString folder);
    QString createUUID();

    bool    uploadCase(ycaTask* task);

    QString errorReason;

protected:

    yctConfiguration* config;

};


#endif // YCTAPI_H
