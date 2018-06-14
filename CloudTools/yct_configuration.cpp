#include "yct_configuration.h"


yctConfiguration::yctConfiguration()
{
    key="";
    secret="";
    region=yctAWSCommon::Regions::us_east_1;
}


bool yctConfiguration::loadConfiguration()
{
    QSettings settings(qApp->applicationDirPath() + YCT_INI_NAME, QSettings::IniFormat);

    QString tempKey   =settings.value("Settings/Value1","").toString();
    QString tempSecret=settings.value("Settings/Value2","").toString();
    int     tempRegion=settings.value("Settings/Value3",0) .toInt();

    key   =QByteArray::fromBase64(tempKey.toLatin1());
    secret=QByteArray::fromBase64(tempSecret.toLatin1());
    region=(yctAWSCommon::Regions) tempRegion;

    return true;
}


bool yctConfiguration::saveConfiguration()
{
    QSettings settings(qApp->applicationDirPath() + YCT_INI_NAME, QSettings::IniFormat);

    QString tempKey   =QString(QByteArray(key.toLatin1()).toBase64());
    QString tempSecret=QString(QByteArray(secret.toLatin1()).toBase64());

    settings.setValue("Settings/Value1", tempKey);
    settings.setValue("Settings/Value2", tempSecret);
    settings.setValue("Settings/Value3", (int) region);

    return true;
}


bool yctConfiguration::isConfigurationValid()
{
    return ((!key.isEmpty()) && (!secret.isEmpty()));
}
