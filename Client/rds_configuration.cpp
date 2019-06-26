#include "rds_configuration.h"
#include "rds_global.h"


rdsConfiguration::rdsConfiguration()
{
    // Read information about the system from the environment variables
    infoSerialNumber   =QProcessEnvironment::systemEnvironment().value("SERIAL_NUMBER",  "0");
    infoScannerType    =QProcessEnvironment::systemEnvironment().value("PRODUCT_NAME",   "Unknown");

    // Read information specifically for NumarisX
    if (QProcessEnvironment::systemEnvironment().value("PRODUCT_NAME","")=="Numaris/X")
    {
        infoSerialNumber=QProcessEnvironment::systemEnvironment().value("SerialNumber",  "0");

        // TODO: Find way to identify system type under NumarisX
        // TODO: Call IDEA command to read system configuration on first run
        infoScannerType ="Vida";
    }

    cloudSupportEnabled=false;
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

    // Used to test if the program has been configured once at all
    infoValidityTest      =settings.value("General/ValidityTest",       false).toBool();

    infoName              =settings.value("General/Name",               "").toString();
    infoShowIcon          =settings.value("General/ShowIcon",           true).toBool();

    infoUpdateMode        =settings.value("General/UpdateMode",         UPDATEMODE_STARTUP_FIXEDTIME).toInt();
    infoUpdatePeriod      =settings.value("General/UpdatePeriod",       6).toInt();
    infoUpdatePeriodUnit  =settings.value("General/UpdatePeriodUnit",   PERIODICUNIT_HOUR).toInt();

    infoUpdateTime1       =settings.value("General/UpdateTime1",        QTime::fromString("03:00:00")).toTime();
    infoUpdateTime2       =settings.value("General/UpdateTime2",        QTime::fromString("06:00:00")).toTime();
    infoUpdateTime3       =settings.value("General/UpdateTime3",        QTime::fromString("12:00:00")).toTime();

    infoUpdateUseTime2    =settings.value("General/UpdateUseTime2",     false).toBool();
    infoUpdateUseTime3    =settings.value("General/UpdateUseTime3",     false).toBool();

    infoJitterTimes       =settings.value("General/JitterTimes",        false).toBool();
    infoJitterWindow      =settings.value("General/JitterWindow",       1).toInt();

    netMode               =settings.value("Network/Mode",               NETWORKMODE_DRIVE).toInt();
    netDriveBasepath      =settings.value("Network/DriveBasepath",      "").toString();
    netDriveReconnectCmd  =settings.value("Network/DriveReconnectCmd",  "").toString();
    netDriveCreateBasepath=settings.value("Network/DriveCreateBasepath",false).toBool();
    netRemoteConfigFile   =settings.value("Network/RemoteConfigFile",   "").toString();

    // Hidden option for rerunning the startup commands if connecting
    // to the network drive failed for three times
    netDriveStartupCmdsAfterFail=settings.value("Network/DriveStartupCmdsAfterFail",false).toBool();

    logServerPath         =settings.value("LogServer/ServerPath",          "").toString();
    logApiKey             =settings.value("LogServer/ApiKey",              "").toString();
    logSendScanInfo       =settings.value("LogServer/SendScanInfo",        true).toBool();
    logSendHeartbeat      =settings.value("LogServer/SendHeartbeat",       true).toBool();
    logUpdateFrequency    =settings.value("LogServer/UpdateFrequency",     4).toInt();
    logUpdateFrequencyUnit=settings.value("LogServer/UpdateFrequencyUnit", 0).toInt();

    startCmds.clear();
    int startCmdsCount    =settings.value("StartCmds/Count",            0).toInt();
    for (int i=0; i<startCmdsCount; i++)
    {
        QString entry=settings.value("StartCmds/Cmd"+QString::number(i),"").toString();
        if (!entry.isEmpty())
        {
            startCmds.append(entry);
        }
    }

    int protocolCount     =settings.value("Protocols/Count",            0).toInt();
    for (int i=0; i<protocolCount; i++)
    {
        QString name       =settings.value("Protocol" + QString::number(i) + "/Name",          "Group"+QString::number(i)).toString();
        QString filter     =settings.value("Protocol" + QString::number(i) + "/Filter",        "_RDS").toString();
        bool saveAdjustData=settings.value("Protocol" + QString::number(i) + "/SaveAdjustData",false).toBool();
        bool anonymizeData =settings.value("Protocol" + QString::number(i) + "/AnonymizeData", false).toBool();
        bool smallFiles    =settings.value("Protocol" + QString::number(i) + "/SmallFiles",    false).toBool();

        addProtocol(name, filter, saveAdjustData, anonymizeData, smallFiles, false);
    }

    infoUpdateTime1Jittered=infoUpdateTime1;
    infoUpdateTime2Jittered=infoUpdateTime2;
    infoUpdateTime3Jittered=infoUpdateTime3;

    // Jitter the update times to avoid that all MRI machines are pushing the
    // raw data at the same time.
    if (infoJitterTimes)
    {
        int jitterSecs=qrand() % (3600 * infoJitterWindow);
        infoUpdateTime1Jittered=infoUpdateTime1Jittered.addSecs(jitterSecs);
        infoUpdateTime2Jittered=infoUpdateTime2Jittered.addSecs(jitterSecs);
        infoUpdateTime3Jittered=infoUpdateTime3Jittered.addSecs(jitterSecs);
    }

    bool remotelyDefinedNameLoaded=loadRemotelyDefinedName();
    if ((!remotelyDefinedNameLoaded) || infoName.isEmpty())
    {
        infoName = infoScannerType+"-"+infoSerialNumber;
    }

    loadCloudSettings();
}


void rdsConfiguration::saveConfiguration()
{
    QSettings settings(RTI->getAppPath() + RDS_INI_NAME, QSettings::IniFormat);

    // Set the configuration validity flag to true to indicate that
    // the configuration has been changed once at least
    settings.setValue("General/ValidityTest",       true);

    settings.setValue("General/Name",               infoName);
    settings.setValue("General/ShowIcon",           infoShowIcon);

    settings.setValue("General/UpdateMode",         infoUpdateMode);
    settings.setValue("General/UpdatePeriod",       infoUpdatePeriod);
    settings.setValue("General/UpdatePeriodUnit",   infoUpdatePeriodUnit);

    settings.setValue("General/UpdateTime1",        infoUpdateTime1);
    settings.setValue("General/UpdateTime2",        infoUpdateTime2);
    settings.setValue("General/UpdateTime3",        infoUpdateTime3);

    settings.setValue("General/UpdateUseTime2",     infoUpdateUseTime2);
    settings.setValue("General/UpdateUseTime3",     infoUpdateUseTime3);

    settings.setValue("General/JitterTimes",        infoJitterTimes);
    settings.setValue("General/JitterWindow",       infoJitterWindow);

    settings.setValue("Network/Mode",               netMode);
    settings.setValue("Network/DriveBasepath",      netDriveBasepath);
    settings.setValue("Network/DriveReconnectCmd",  netDriveReconnectCmd);
    settings.setValue("Network/DriveCreateBasepath",netDriveCreateBasepath);
    settings.setValue("Network/RemoteConfigFile",   netRemoteConfigFile);
    settings.setValue("Network/DriveStartupCmdsAfterFail",netDriveStartupCmdsAfterFail);

    settings.setValue("LogServer/ServerPath",         logServerPath);
    settings.setValue("LogServer/ApiKey",             logApiKey);
    settings.setValue("LogServer/SendHeartbeat",      logSendHeartbeat);
    settings.setValue("LogServer/SendScanInfo",       logSendScanInfo);
    settings.setValue("LogServer/UpdateFrequency",    logUpdateFrequency);
    settings.setValue("LogServer/UpdateFrequencyUnit",logUpdateFrequencyUnit);

    settings.setValue("StartCmds/Count",            startCmds.count());
    for (int i=0; i<startCmds.count(); i++)
    {
        settings.setValue("StartCmds/Cmd"+QString::number(i), startCmds.at(i));
    }

    settings.setValue("Protocols/Count",            getProtocolCount());
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


bool rdsConfiguration::loadRemotelyDefinedProtocols()
{
    // First, remove the already loaded remote procotols, so that they
    // are replaced with the newer ones from the remote file
    removeRemotelyDefinedProtocols();

    // Return if remote protocols have not been defined
    if (netRemoteConfigFile.isEmpty())
    {
        return true;
    }

    if (!QFile::exists(netRemoteConfigFile))
    {
        RTI->log("ERROR: Unable to find remote configuration file.");
        return false;
    }

    QSettings settings(netRemoteConfigFile, QSettings::IniFormat);

    // Used to test if the program has been configured once at all
    bool validityTest=settings.value("Yarra/RemoteConfiguration",false).toBool();

    if (!validityTest)
    {
        RTI->log("WARNING: Remote configuration file not valid.");
        return true;
    }

    QStringList sections=settings.childGroups();

    int addedProtocols=0;

    for (int i=0; i<sections.count(); i++)
    {
        if (sections.at(i)!="Yarra")
        {
            QString name=sections.at(i);
            QString filter     =settings.value(name+"/Filter",       "_RDS").toString();
            bool saveAdjustData=settings.value(name+"/SaveAdjustData",false).toBool();
            bool anonymizeData =settings.value(name+"/AnonymizeData", false).toBool();
            bool smallFiles    =settings.value(name+"/SmallFiles",    false).toBool();

            bool addThisProtocol=true;

            const QString invalidStr="#@4#4@";
            QStringList includeSystem=settings.value(name+"/IncludeSystems",invalidStr).toStringList();
            QStringList excludeSystem=settings.value(name+"/ExcludeSystems",invalidStr).toStringList();

            if ((includeSystem.count()==1) && (includeSystem.at(0)==invalidStr))
            {
                includeSystem.clear();
            }
            if ((excludeSystem.count()==1) && (excludeSystem.at(0)==invalidStr))
            {
                excludeSystem.clear();
            }

            // If a list of include systems is provided, check if this sytem is listed
            if (!includeSystem.isEmpty())
            {
                if ((!includeSystem.contains(infoSerialNumber)) && (!includeSystem.contains(infoName)))
                {
                    addThisProtocol=false;
                }
            }

            // If a list of exclude systems is provided, check if this sytem is on the list
            if ((excludeSystem.contains(infoSerialNumber)) || (excludeSystem.contains(infoName)))
            {
                addThisProtocol=false;
            }

            if (addThisProtocol)
            {
                addProtocol(name, filter, saveAdjustData, anonymizeData, smallFiles, true);
                addedProtocols++;
            }
        }        
    }

    if (addedProtocols>0)
    {
        RTI->log(QString::number(addedProtocols)+" remote protocols added.");
    }

    return true;
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


bool rdsConfiguration::loadRemotelyDefinedName()
{
    if (netRemoteConfigFile.isEmpty())
    {
        return true;
    }

    if (!QFile::exists(netRemoteConfigFile))
    {
        RTI->log("ERROR: Unable to find remote configuration file.");
        return false;
    }

    QSettings settings(netRemoteConfigFile, QSettings::IniFormat);

    QString name=settings.value("ScannerNames/"+infoSerialNumber).toString();
    if (name.isEmpty())
    {
        return false;
    }

    infoName=name;

    return true;
}


void rdsConfiguration::loadCloudSettings()
{
    QSettings ortSettings(RTI->getAppPath() + "/ort.ini", QSettings::IniFormat);
    cloudSupportEnabled=ortSettings.value("ORT/CloudSupport",false).toBool();
}
