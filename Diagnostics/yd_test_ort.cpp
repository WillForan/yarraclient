#include "yd_test_ort.h"
#include "yd_global.h"

ydTestORT::ydTestORT() : ydTest()
{
}


QString ydTestORT::getName()
{
    return "Offline Reconstruction";
}


QString ydTestORT::getDescription()
{
    return "indentify ORT-related problems";
}


bool ydTestORT::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION
    if (broker->contains(YD_KEY_NO_ORT_AVAILABLE))
    {
        YD_ADDRESULT("Skipping ORT tests (not configured)");
        YD_RESULT_ENDSECTION
        return true;
    }

    ortConfig.loadConfiguration();
    if (!ortConfig.isConfigurationValid())
    {
        YD_ADDRESULT_COLORLINE("ORT configuration file is invalid", YD_CRITICAL);
        YD_ADDISSUE("ORT configuration file invalid",YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return true;
    }

    YD_ADDRESULT("ORT system name: " + ortConfig.ortSystemName);
    if ((ortConfig.ortSystemName.isEmpty()) || (ortConfig.ortSystemName=="NotGiven"))
    {
        YD_ADDRESULT_COLORLINE("ORT system name has not been defined", YD_WARNING);
        YD_ADDISSUE("ORT system name not defined",YD_WARNING);
    }

    YD_RESULT_ENDSECTION
    return true;
}

