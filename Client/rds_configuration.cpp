#include "rds_configuration.h"
#include "rds_global.h"


rdsConfiguration::rdsConfiguration()
{
    infoSerialNumber=QProcessEnvironment::systemEnvironment().value("SERIAL_NUMBER","0");
}


rdsConfiguration::~rdsConfiguration()
{
    // Delete all items from the protocol list
    while (!protocols.isEmpty())
    {
        delete protocols.takeFirst();
    }
}


bool rdsConfiguration::isConfigurationValid()
{
    return infoValidityTest;
}


void rdsConfiguration::loadConfiguration()
{
    QSettings settings(RTI->getAppPath() + RDS_INI_NAME, QSettings::IniFormat);

    // Load the configuration values from the configuration file
    // If the configuration file does not exist, the defailt values will be used.
    infoName            =settings.value("General/Name", "Unnamed").toString();
    infoShowIcon        =settings.value("General/ShowIcon", true).toBool();

    infoUpdateMode      =settings.value("General/UpdateMode", UPDATEMODE_STARTUP_FIXEDTIME).toInt();
    infoUpdatePeriod    =settings.value("General/UpdatePeriod", 6).toInt();
    infoUpdatePeriodUnit=settings.value("General/UpdatePeriodUnit", PERIODICUNIT_HOUR).toInt();

    infoUpdateTime1     =settings.value("General/UpdateTime1", QTime::fromString("03:00:00")).toTime();
    infoUpdateTime2     =settings.value("General/UpdateTime2", QTime::fromString("06:00:00")).toTime();
    infoUpdateTime3     =settings.value("General/UpdateTime3", QTime::fromString("12:00:00")).toTime();

    infoUpdateUseTime2  =settings.value("General/UpdateUseTime2", false).toBool();
    infoUpdateUseTime3  =settings.value("General/UpdateUseTime3", false).toBool();

    netMode               =settings.value("Network/Mode", NETWORKMODE_FTP).toInt();
    netDriveBasepath      =settings.value("Network/DriveBasepath", "").toString();
    netDriveReconnectCmd  =settings.value("Network/DriveReconnectCmd", "").toString();
    netDriveCreateBasepath=settings.value("Network/DriveCreateBasepath", false).toBool();

    netFTPIP            =settings.value("Network/FTPIP", "123.123.123.123").toString();
    netFTPBasepath      =settings.value("Network/FTPBasepath", "/rawdata").toString();
    netFTPUser          =settings.value("Network/FTPUser", "rdsuser").toString();
    netFTPPassword      =settings.value("Network/FTPPassword", "rdspwd").toString(); // TODO: Scramble password!

    logServerPath       =settings.value("LogServer/ServerPath","").toString();
    logSendScanInfo     =settings.value("LogServer/SendScanInfo", true).toBool();
    logUpdateFrequency  =settings.value("LogServer/UpdateFrequency", 4).toInt();

    // Used to test if the program has been configured once at all
    infoValidityTest    =settings.value("General/ValidityTest", false).toBool();

    int protocolCount   =settings.value("Protocols/Count", 0).toInt();

    for (int i=0; i<protocolCount; i++)
    {
        QString name=  settings.value("Protocol" + QString::number(i) + "/Name",   "Group"+QString::number(i) ).toString();
        QString filter=settings.value("Protocol" + QString::number(i) + "/Filter", "_RDS" ).toString();
        bool saveAdjustData=settings.value("Protocol" + QString::number(i) + "/SaveAdjustData", false ).toBool();
        bool anonymizeData= settings.value("Protocol" + QString::number(i) + "/AnonymizeData",  false ).toBool();
        bool smallFiles= settings.value("Protocol" + QString::number(i) + "/SmallFiles",  false ).toBool();

        addProtocol(name, filter, saveAdjustData, anonymizeData, smallFiles, false);
    }
}


void rdsConfiguration::saveConfiguration()
{
    QSettings settings(RTI->getAppPath() + RDS_INI_NAME, QSettings::IniFormat);

    settings.setValue("General/Name", infoName);
    settings.setValue("General/ShowIcon", infoShowIcon);

    settings.setValue("General/UpdateMode", infoUpdateMode);
    settings.setValue("General/UpdatePeriod", infoUpdatePeriod);
    settings.setValue("General/UpdatePeriodUnit", infoUpdatePeriodUnit);

    settings.setValue("General/UpdateTime1", infoUpdateTime1);
    settings.setValue("General/UpdateTime2", infoUpdateTime2);
    settings.setValue("General/UpdateTime3", infoUpdateTime3);

    settings.setValue("General/UpdateUseTime2", infoUpdateUseTime2);
    settings.setValue("General/UpdateUseTime3", infoUpdateUseTime3);

    settings.setValue("Network/Mode", netMode);
    settings.setValue("Network/DriveBasepath", netDriveBasepath);
    settings.setValue("Network/DriveReconnectCmd", netDriveReconnectCmd);
    settings.setValue("Network/DriveCreateBasepath", netDriveCreateBasepath);

    settings.setValue("LogServer/ServerPath",  logServerPath);
    settings.setValue("LogServer/SendScanInfo",logSendScanInfo);
    settings.setValue("LogServer/UpdateFrequency",logUpdateFrequency);

    settings.setValue("Network/FTPIP", netFTPIP);
    settings.setValue("Network/FTPBasepath", netFTPBasepath);
    settings.setValue("Network/FTPUser", netFTPUser);
    settings.setValue("Network/FTPPassword", netFTPPassword);  // TODO: Scramble password!

    // Set the configuration validity flag to true to indicate that the configuration
    // has been changed once at least
    settings.setValue("General/ValidityTest", true);

    settings.setValue("Protocols/Count", getProtocolCount());

    for (int i=0; i<getProtocolCount(); i++)
    {
        QString name="";
        QString filter="";
        bool saveAdjustData=false;
        bool anonymizeData=false;
        bool smallFiles=false;
        bool remotelyDefined=false;

        // Read settings from protocol list
        readProtocol(i, name, filter, saveAdjustData, anonymizeData, smallFiles, remotelyDefined);

        if (!remotelyDefined)
        {
            // Write to ini file
            settings.setValue("Protocol" + QString::number(i) + "/Name",           name);
            settings.setValue("Protocol" + QString::number(i) + "/Filter",         filter);
            settings.setValue("Protocol" + QString::number(i) + "/SaveAdjustData", saveAdjustData);
            settings.setValue("Protocol" + QString::number(i) + "/AnonymizeData",  anonymizeData);
            settings.setValue("Protocol" + QString::number(i) + "/SmallFiles",     smallFiles);
        }
    }
}


void rdsConfiguration::addProtocol(QString name, QString filter, bool saveAdjustData, bool anonymizeData, bool smallFiles, bool remotelyDefined)
{
    rdsConfigurationProtocol* newItem=new rdsConfigurationProtocol;

    newItem->name=name;
    newItem->filter=filter;
    newItem->saveAdjustData=saveAdjustData;
    newItem->anonymizeData=anonymizeData;
    newItem->smallFiles=smallFiles;
    newItem->remotelyDefined=remotelyDefined;

    protocols.append(newItem);
}


void rdsConfiguration::updateProtocol(int index, QString name, QString filter, bool saveAdjustData, bool anonymizeData, bool smallFiles, bool remotelyDefined)
{
    rdsConfigurationProtocol* item=protocols.at(index);

    if (item!=0)
    {
        item->name=name;
        item->filter=filter;
        item->saveAdjustData=saveAdjustData;
        item->anonymizeData=anonymizeData;
        item->smallFiles=smallFiles;
        item->remotelyDefined=remotelyDefined;
    }
}


void rdsConfiguration::readProtocol(int index, QString& name, QString& filter, bool& saveAdjustData, bool& anonymizeData, bool &smallFiles, bool remotelyDefined)
{
    rdsConfigurationProtocol* item=protocols.at(index);

    if (item!=0)
    {
        name=item->name;
        filter=item->filter;
        saveAdjustData=item->saveAdjustData;
        anonymizeData=item->anonymizeData;
        smallFiles=item->smallFiles;
        remotelyDefined=item->remotelyDefined;
    }
}


void rdsConfiguration::deleteProtocol(int index)
{
    rdsConfigurationProtocol* item=protocols.at(index);

    if (item!=0)
    {
        delete item;
        item=0;

        protocols.removeAt(index);
    }
}


void rdsConfiguration::removeRemotelyDefinedProtocols()
{
    int itemCount=protocols.count();

    for (int i=itemCount-1; i>=0; i--)
    {
        rdsConfigurationProtocol* item=protocols.at(i);

        if (item!=0)
        {
            if (item->remotelyDefined)
            {
                protocols.removeAt(i);
                delete item;
                item=0;
            }
        }
    }
}
