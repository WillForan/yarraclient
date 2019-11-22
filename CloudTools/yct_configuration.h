#ifndef YCTCONFIGURATION_H
#define YCTCONFIGURATION_H

#include <QtCore>
#include "yct_common.h"


class yctConfiguration
{
public:
    yctConfiguration();

    QString key;
    QString secret;
    bool    showNotifications;

    QString proxyIP;
    int     proxyPort;
    QString proxyUsername;
    QString proxyPassword;

    bool loadConfiguration();
    bool saveConfiguration();

    bool isConfigurationValid();
};


#endif // YCTCONFIGURATION_H
