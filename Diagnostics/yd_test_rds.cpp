#include "yd_test_rds.h"

#include "yd_global.h"
#include "yd_helper.h"

#include <QDir>


ydTestRDS::ydTestRDS() : ydTest()
{
}


QString ydTestRDS::getName()
{
    return "Raw Data Storage";
}


QString ydTestRDS::getDescription()
{
    return "indentify RDS-related problems";
}


bool ydTestRDS::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION

    if (broker->contains(YD_KEY_NO_RDS_AVAILABLE))
    {
        YD_ADDRESULT("Skipping RDS tests (not configured)");
        YD_RESULT_ENDSECTION
        return true;
    }

    rdsConfig.loadConfiguration();
    if (!rdsConfig.isConfigurationValid())
    {
        YD_ADDRESULT_COLORLINE("RDS configuration file is invalid", YD_CRITICAL);
        YD_ADDISSUE("RDS configuration file invalid", YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return true;
    }

    YD_ADDRESULT("RDS system name: " + rdsConfig.infoName);
    if ((rdsConfig.infoName.isEmpty()) || (rdsConfig.infoName=="NotGiven"))
    {
        YD_ADDRESULT_COLORLINE("RDS system name has not been defined", YD_WARNING);
        YD_ADDISSUE("RDS system name not defined", YD_WARNING);
    }

    if ((!rdsConfig.logServerPath.isEmpty()) && (!rdsConfig.logApiKey.isEmpty()))
    {
        broker->insert("logserver_address", rdsConfig.logServerPath);
        broker->insert("logserver_apikey", rdsConfig.logApiKey);
        broker->insert("logserver_source", "RDS");
    }

    // TODO: Open connection and check for storage destination
    // TODO: Check for available disk space and warn if low
    // TODO: Test write permissions

    return true;
}

