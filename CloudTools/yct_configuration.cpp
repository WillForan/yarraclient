#include "yct_configuration.h"


yctConfiguration::yctConfiguration()
{
    key="";
    secret="";
    showNotifications=true;

    proxyIP      ="";
    proxyPort    =8000;
    proxyUsername="";
    proxyPassword="";
}


bool yctConfiguration::loadConfiguration()
{
    QSettings settings(qApp->applicationDirPath() + YCT_INI_NAME, QSettings::IniFormat);

    QString tempKey   =settings.value("Settings/Value1","").toString();
    QString tempSecret=settings.value("Settings/Value2","").toString();
    showNotifications =settings.value("Settings/ShowNotifications",true).toBool();

    key   =QByteArray::fromBase64(tempKey.toLatin1());
    secret=QByteArray::fromBase64(tempSecret.toLatin1());

    proxyIP      =settings.value("Proxy/IP","").toString();
    proxyPort    =settings.value("Proxy/Port",8000).toInt();
    proxyUsername=settings.value("Proxy/Username","").toString();
    proxyPassword=settings.value("Proxy/Password","").toString();

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

    settings.setValue("Proxy/IP",       proxyIP);
    settings.setValue("Proxy/Port",     proxyPort);
    settings.setValue("Proxy/Username", proxyUsername);
    settings.setValue("Proxy/Password", proxyPassword);

    return true;
}


bool yctConfiguration::isConfigurationValid()
{
    return ((!key.isEmpty()) && (!secret.isEmpty()));
}
