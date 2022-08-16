#include "yd_test_rds.h"


ydTestRDS::ydTestRDS() : ydTest()
{
}


QString ydTestRDS::getName()
{
    return "Raw Data Storage";
}


QString ydTestRDS::getDescription()
{
    return "indentify RDS-related problems";
}


bool ydTestRDS::run(QString& issues, QString& results)
{
    return true;
}

