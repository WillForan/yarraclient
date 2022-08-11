#include "yd_test_systeminfo.h"

ydTestSysteminfo::ydTestSysteminfo()
{

}


QString ydTestSysteminfo::getName()
{
    return "System Information";
}


QString ydTestSysteminfo::getDescription()
{
    return "General information about the system";
}


QString ydTestSysteminfo::getIssues()
{
    return "";
}


QString ydTestSysteminfo::getResults()
{
    return "Time is now "+QDateTime::currentDateTime().toString();
}


bool ydTestSysteminfo::run()
{
    QThread::sleep(3);

    return true;
}
