#ifndef RDS_CONFIGURATION_H
#define RDS_CONFIGURATION_H

#include <Qtgui>


class rdsConfigurationProtocol
{
public:
    QString name;
    QString filter;
    bool saveAdjustData;
    bool anonymizeData;
    bool smallFiles;
    bool remotelyDefined;
};


class rdsConfiguration
{
public:
    rdsConfiguration();
    ~rdsConfiguration();

    void loadConfiguration();
    void saveConfiguration();

    bool isConfigurationValid();

    bool    infoValidityTest;

    QString infoName;
    QString infoSerialNumber;
    int     infoUpdateMode;
    int     infoUpdatePeriod;
    int     infoUpdatePeriodUnit;
    QTime   infoUpdateTime1;
    QTime   infoUpdateTime2;
    QTime   infoUpdateTime3;
    bool    infoUpdateUseTime2;
    bool    infoUpdateUseTime3;

    int     netMode;
    QString netFTPIP;
    QString netFTPUser;
    QString netFTPPassword;
    QString netFTPBasepath;

    QString netDriveBasepath;
    QString netDriveReconnectCmd;
    bool    netDriveCreateBasepath;

    QString logServerPath;
    bool    logSendScanInfo;
    QString logServerKey;

    QList<rdsConfigurationProtocol*> protocols;
    int getProtocolCount();
    void addProtocol(QString name, QString filter, bool saveAdjustData, bool anonymizeData, bool smallFiles, bool remotelyDefined);
    void updateProtocol(int index, QString name, QString filter, bool saveAdjustData, bool anonymizeData, bool smallFiles, bool remotelyDefined);
    void readProtocol(int index, QString& name, QString& filter, bool& saveAdjustData, bool& anonymizeData, bool &smallFiles, bool remotelyDefined);
    void deleteProtocol(int index);

    void removeRemotelyDefinedProtocols();

    bool protocolNeedsAnonymization(int index);

    enum
    {
        UPDATEMODE_STARTUP          =0,
        UPDATEMODE_FIXEDTIME        =1,
        UPDATEMODE_PERIODIC         =2,
        UPDATEMODE_MANUAL           =3,
        UPDATEMODE_STARTUP_FIXEDTIME=4
    };

    enum
    {
        PERIODICUNIT_MIN  =0,
        PERIODICUNIT_HOUR =1
    };

    enum
    {
        NETWORKMODE_DRIVE=0,
        NETWORKMODE_FTP  =1
    };

    bool isNetworkModeFTP();
    bool isNetworkModeDrive();

    bool isLogServerConfigured();

protected:

};



inline int rdsConfiguration::getProtocolCount()
{
    return protocols.count();
}


inline bool rdsConfiguration::isNetworkModeFTP()
{
    return netMode==NETWORKMODE_FTP;
}


inline bool rdsConfiguration::isNetworkModeDrive()
{
    return netMode==NETWORKMODE_DRIVE;
}


inline bool rdsConfiguration::protocolNeedsAnonymization(int index)
{
    rdsConfigurationProtocol* item=protocols.at(index);

    if (item!=0)
    {
        return (item->anonymizeData);
    }
    else
    {
        return false;
    }
}


inline bool rdsConfiguration::isLogServerConfigured()
{
    return (!logServerPath.isEmpty());
}



#endif // RDS_CONFIGURATION_H


