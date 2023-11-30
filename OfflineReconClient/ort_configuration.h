#ifndef ORT_CONFIGURATION_H
#define ORT_CONFIGURATION_H

#include <QtCore>


class ortConfiguration
{
public:
    ortConfiguration();
    ~ortConfiguration();

    void loadConfiguration();
    void saveConfiguration();

    bool isConfigurationValid();

    QString     ortSystemName;
    QString     ortServerPath;
    QString     ortConnectCmd;
    QString     ortDisconnectCmd;
    QString     ortFallbackConnectCmd;
    int         ortConnectTimeout;    
    QStringList ortMailPresets;
    bool        ortStartRDSOnShutdown;
    bool        ortCloudSupportEnabled;
    QString     ortServerType;
    QString     logServerAddress;
    QString     logServerAPIKey;

    QString     infoSerialNumber;
    QString     infoScannerType;
    QString     infoSoftwareVersion;

};

#endif // ORT_CONFIGURATION_H

