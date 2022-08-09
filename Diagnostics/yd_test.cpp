#include "yd_test.h"
#include "yd_global.h"


ydTest::ydTest()
{
}


QString ydTest::getName()
{
    return "Base";
}


QString ydTest::getDescription()
{
    return "";
}


QString ydTest::getResultsHTML()
{
    return "OK";
}


QString ydTest::getIssuesHTML()
{
    return "Nothing";
}


QString ydTest::getResultsText()
{
    return "OK";
}


bool ydTest::run()
{
    return true;
}




ydTestList::ydTestList()
{
    testResultHTML="";
}


bool ydTestList::runAllTests()
{
    testResultHTML="";

    for (int i=0; i<this->count(); i++)
    {


        this->at(i)->getName();
    }

    return true;
}
