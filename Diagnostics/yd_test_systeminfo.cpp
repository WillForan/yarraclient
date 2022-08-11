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


QString ydTestSysteminfo::getIssuesHTML()
{
    return "";
}


QString ydTestSysteminfo::getResultsHTML()
{
    return "";
}


QString ydTestSysteminfo::getResultsText()
{
    return "";
}

bool ydTestSysteminfo::run()
{
    qInfo() << "PING";
    QThread::sleep(3);
    qInfo() << "PONG";

    return true;
}
