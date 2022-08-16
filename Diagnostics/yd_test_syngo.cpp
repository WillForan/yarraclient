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
        broker->insert(YD_KEY_NO_SYNGO_AVAILABLE, "true");
        YD_RESULT_ENDSECTION
        return true;
    }

    QString serialNumber=QProcessEnvironment::systemEnvironment().value("SERIAL_NUMBER", "0");
    QString scannerType =QProcessEnvironment::systemEnvironment().value("PRODUCT_NAME",  "Unknown");

    // On NumarisX, a different key is used to store the serial number
    if (scannerType=="Numaris/X")
    {
        serialNumber=QProcessEnvironment::systemEnvironment().value("SerialNumber", "0");
    }

    YD_ADDRESULT("Serial number: " + serialNumber);
    YD_ADDRESULT("Product name: " + scannerType);

    YD_RESULT_ENDSECTION

    return true;
}
