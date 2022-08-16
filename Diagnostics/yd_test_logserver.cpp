#include "yd_test_logserver.h"


ydTestLogServer::ydTestLogServer() : ydTest()
{
}


QString ydTestLogServer::getName()
{
    return "LogServer";
}


QString ydTestLogServer::getDescription()
{
    return "indentify LogServer issues";
}


bool ydTestLogServer::run(QString& issues, QString& results)
{
    return true;
}

