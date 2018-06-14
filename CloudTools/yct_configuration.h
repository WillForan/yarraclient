#ifndef YCTCONFIGURATION_H
#define YCTCONFIGURATION_H

#include <QtCore>
#include "yct_common.h"


class yctConfiguration
{
public:
    yctConfiguration();

    QString               key;
    QString               secret;
    yctAWSCommon::Regions region;

    bool loadConfiguration();
    bool saveConfiguration();

    QString getRegion();
    void setRegion(int newRegion);
    int getRegionInt();

    bool isConfigurationValid();

};


inline QString yctConfiguration::getRegion()
{
    return yctAWSCommon::getRegionID(region);
}


inline int yctConfiguration::getRegionInt()
{
    int regionInt=(int) region;
    return regionInt;
}


inline void yctConfiguration::setRegion(int newRegion)
{
    if ((newRegion<0) || (newRegion>=yctAWSCommon::regionCount))
    {
        return;
    }

    region=(yctAWSCommon::Regions) newRegion;
}




#endif // YCTCONFIGURATION_H
