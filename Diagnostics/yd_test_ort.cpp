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
    if (broker->contains(YD_KEY_NO_ORT_AVAILABLE))
    {
        YD_RESULT_STARTSECTION
        YD_ADDRESULT("Skipping ORT tests (not configured)");
        YD_RESULT_ENDSECTION
        return true;
    }

    ortConfig.loadConfiguration();

    return true;
}

