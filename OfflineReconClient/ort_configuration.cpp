#include "ort_configuration.h"
#include "ort_global.h"



ortConfiguration::ortConfiguration()
{
    ortSystemName=ORT_INVALID;
}


ortConfiguration::~ortConfiguration()
{
    ortMailPresets.clear();
}


bool ortConfiguration::isConfigurationValid()
{
    return (ortSystemName!=ORT_INVALID);
}


void ortConfiguration::loadConfiguration()
{
    QSettings settings(RTI->getAppPath()+"/"+ORT_INI_NAME, QSettings::IniFormat);

    // Used to test if the program has been configured once at all
    ortSystemName=settings.value("ORT/SystemName", QString(ORT_INVALID)).toString();
    ortServerPath=settings.value("ORT/ServerPath", "").toString();
    ortConnectCmd=settings.value("ORT/ConnectCmd", "").toString();
    ortDisconnectCmd=settings.value("ORT/DisconnectCmd", "").toString();
    ortFallbackConnectCmd=settings.value("ORT/FallbackConnectCmd", "").toString();
    ortConnectTimeout=settings.value("ORT/ConnectTimeout", 0).toInt();

    // Read the mail presets for the ORT configuration dialog
    ortMailPresets.clear();
    int mCount=1;
    while ((!settings.value("ORT/MailPreset"+QString::number(mCount),"").toString().isEmpty()) && (mCount<100))
    {
        ortMailPresets.append(settings.value("ORT/MailPreset"+QString::number(mCount),"").toString());
        mCount++;
    }
}


void ortConfiguration::saveConfiguration()
{
    QSettings settings(RTI->getAppPath()+"/"+ORT_INI_NAME, QSettings::IniFormat);

    settings.setValue("ORT/SystemName", ortSystemName);
    settings.setValue("ORT/ServerPath", ortServerPath);
    settings.setValue("ORT/ConnectCmd", ortConnectCmd);
    settings.setValue("ORT/DisconnectCmd", ortDisconnectCmd);
    settings.setValue("ORT/FallbackConnectCmd", ortFallbackConnectCmd);
    settings.setValue("ORT/ConnectTimeout", ortConnectTimeout);

    // Read the mail presets for the ORT configuration dialog
    for (int i=0; i<ortMailPresets.count(); i++)
    {
        settings.setValue("ORT/MailPreset"+QString::number(i+1), ortMailPresets.at(i));
    }
}
