#include "yct_configuration.h"


yctConfiguration::yctConfiguration()
{
    key="";
    secret="";
    showNotifications=true;
}


bool yctConfiguration::loadConfiguration()
{
    QSettings settings(qApp->applicationDirPath() + YCT_INI_NAME, QSettings::IniFormat);

    QString tempKey   =settings.value("Settings/Value1","").toString();
    QString tempSecret=settings.value("Settings/Value2","").toString();
    showNotifications =settings.value("Settings/ShowNotifications",true).toBool();

    key   =QByteArray::fromBase64(tempKey.toLatin1());
    secret=QByteArray::fromBase64(tempSecret.toLatin1());

    return true;
}


bool yctConfiguration::saveConfiguration()
{
    QSettings settings(qApp->applicationDirPath() + YCT_INI_NAME, QSettings::IniFormat);

    QString tempKey   =QString(QByteArray(key.toLatin1()).toBase64());
    QString tempSecret=QString(QByteArray(secret.toLatin1()).toBase64());

    settings.setValue("Settings/Value1", tempKey);
    settings.setValue("Settings/Value2", tempSecret);
    settings.setValue("Settings/ShowNotifications",showNotifications);

    return true;
}


bool yctConfiguration::isConfigurationValid()
{
    return ((!key.isEmpty()) && (!secret.isEmpty()));
}
