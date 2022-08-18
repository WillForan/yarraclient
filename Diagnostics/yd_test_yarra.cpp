#include "yd_test_yarra.h"
#include "yd_global.h"

#include "../Client/rds_runtimeinformation.h"

#include <QSettings>


ydTestYarra::ydTestYarra() : ydTest()
{
}


QString ydTestYarra::getName()
{
    return "Yarra";
}


QString ydTestYarra::getDescription()
{
    return "find Yarra installation issues";
}


bool ydTestYarra::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION
    QString installationPath = qApp->applicationDirPath();
    YD_ADDRESULT("Yarra installation path: " + installationPath);
    YD_RESULT_ENDSECTION

    YD_RESULT_STARTSECTION
    YD_ADDRESULT("Build version numbers:");
    YD_ADDRESULT_LINE(" - RDS version: " + QString(RDS_VERSION));
    YD_ADDRESULT_LINE(" - ORT version: " + QString(ORT_VERSION));
    YD_ADDRESULT_LINE(" - YCA version: " + QString(YCA_VERSION));
    YD_RESULT_ENDSECTION

    YD_RESULT_STARTSECTION
    YD_ADDRESULT("Checking installation integrity");

    // Check if all Yarra components are existing
    QDir yarraDir(installationPath);
    if (!yarraDir.exists("RDS.exe"))
    {
        YD_ADDRESULT_COLORLINE("Binary RDS.exe missing in installation path", YD_CRITICAL);
        YD_ADDISSUE("Binary file RDS.exe missing",YD_CRITICAL);
    }

    if (!yarraDir.exists("ORT.exe"))
    {
        YD_ADDRESULT_COLORLINE("Binary ORT.exe missing in installation path", YD_CRITICAL);
        YD_ADDISSUE("Binary file ORT.exe missing",YD_CRITICAL);
    }

    if (!yarraDir.exists("YCA.exe"))
    {
        YD_ADDRESULT_COLORLINE("Binary YCA.exe missing in installation path", YD_WARNING);
        YD_ADDISSUE("Binary file YCA.exe missing",YD_WARNING);
    }

    if (!yarraDir.exists("YCA_helper.exe"))
    {
        YD_ADDRESULT_COLORLINE("Binary YCA_helper.exe missing in installation path", YD_WARNING);
        YD_ADDISSUE("Binary file YCA_helper.exe missing",YD_WARNING);
    }

    if (!yarraDir.exists("ort.ini"))
    {
        YD_ADDRESULT_COLORLINE("ORT configuration file not found", YD_WARNING);
        YD_ADDRESULT_COLORLINE("ORT will not be able to run", YD_WARNING);
        YD_ADDISSUE("ORT configuration file not found", YD_WARNING);
        broker->insert(YD_KEY_NO_ORT_AVAILABLE, "true");
    }

    if (!yarraDir.exists("rds.ini"))
    {
        YD_ADDRESULT_COLORLINE("RDS configuration file not found", YD_WARNING);
        YD_ADDRESULT_COLORLINE("RDS will not be able to run", YD_WARNING);
        YD_ADDISSUE("RDS configuration file not found", YD_WARNING);
        broker->insert(YD_KEY_NO_RDS_AVAILABLE, "true");
    }

    YD_RESULT_ENDSECTION
    return true;
}


