#include "yd_test_systeminfo.h"
#include "yd_global.h"


ydTestSysteminfo::ydTestSysteminfo() : ydTest()
{
}


QString ydTestSysteminfo::getName()
{
    return "System Info";
}


QString ydTestSysteminfo::getDescription()
{
    return "collect general system information";
}


bool ydTestSysteminfo::run(QString& issues, QString& results)
{


    return true;
}
