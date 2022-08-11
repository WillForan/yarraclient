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



ysTestThread::ysTestThread(ydTestRunner* myParent)
{
    runner=myParent;
    currentIndex=0;
}


void ysTestThread::run()
{
    runner->testResultHTML="";

    qInfo() << "HERE";

    for (int i=0; i<runner->testList.count(); i++)
    {
        qInfo() << "THIS";

        if (isInterruptionRequested())
        {
            break;
        }

        currentIndex=i;
        runner->testList.at(i)->run();
        //this->at(i)->getName();

        if (isInterruptionRequested())
        {
            break;
        }
    }

    runner->isActive=false;
    quit();

}


ydTestRunner::ydTestRunner() : testThread(this)
{
    testResultHTML="";
    isActive=false;
}


bool ydTestRunner::runTests()
{
    if (isActive)
    {
        return false;
    }
    isActive=true;
    testThread.start();
    return true;
}


bool ydTestRunner::cancelTests()
{
    if (!isActive)
    {
        return false;
    }
    testThread.requestInterruption();
    return true;
}


int ydTestRunner::getPercentage()
{
    if (testList.isEmpty())
    {
        return 0;
    }

    return int(100.*(testThread.currentIndex)/double(testList.count()));
}

