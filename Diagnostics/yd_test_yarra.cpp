#include "yd_test_yarra.h"
#include "yd_global.h"

#include "../Client/rds_runtimeinformation.h"


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
    YD_ADDRESULT("Yarra installation path: ");

    return true;
}


