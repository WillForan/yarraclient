#ifndef YCTAPI_H
#define YCTAPI_H

class yctConfiguration;
class ortModeList;


class yctAPI
{
public:

    yctAPI();
    void setConfiguration(yctConfiguration* configuration);

    int readModeList(ortModeList* modeList);

    void launchCloudAgent();

protected:

    yctConfiguration* config;

};


#endif // YCTAPI_H
