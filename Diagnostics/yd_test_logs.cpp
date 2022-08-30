#include "yd_test_logs.h"
#include "yd_global.h"


ydTestLogs::ydTestLogs() : ydTest()
{
}


QString ydTestLogs::getName()
{
    return "Logs";
}


QString ydTestLogs::getDescription()
{
    return "captures latest log output";
}


bool ydTestLogs::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION
    YD_RESULT_ENDSECTION
    return true;
}
