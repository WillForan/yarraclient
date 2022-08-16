#include "yd_test_syngo.h"
#include "yd_global.h"

#include "../Client/rds_runtimeinformation.h"

ydTestSyngo::ydTestSyngo() : ydTest()
{
}


QString ydTestSyngo::getName()
{
    return "Syngo";
}


QString ydTestSyngo::getDescription()
{
    return "indentify installed Siemens software";
}


bool ydTestSyngo::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION

    YD_ADDRESULT("Checking Syngo installation")

    QDir syngoDir("C:\\Medcom");
    if (!syngoDir.exists())
    {
        YD_ADDRESULT_COLORLINE("No Syngo folder found. Not running on scanner.", YD_INFO);
        YD_ADDISSUE("No Syngo installation found", YD_INFO);
        broker->insert("no_syngo_available", "true");
    }

    YD_RESULT_ENDSECTION

    return true;
}
