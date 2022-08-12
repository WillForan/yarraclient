#include "yd_test.h"
#include "yd_global.h"


ydTest::ydTest()
{
    broker=0;
}


QString ydTest::getName()
{
    return "Base";
}


QString ydTest::getDescription()
{
    return "";
}


bool ydTest::run(QString& issues, QString& results)
{
    return true;
}


void ydTest::setBroker(ydBroker* brokerInstance)
{
    broker=brokerInstance;
}


ysTestThread::ysTestThread(ydTestRunner* myParent)
{
    runner=myParent;
    currentIndex=0;
}


void ysTestThread::run()
{
    bool cancelled=false;

    runner->results="<p>Starting diagnostics at "+QDateTime::currentDateTime().toString()+"</p>";
    runner->issues="";
    runner->broker.clear();

    for (int i=0; i<runner->testList.count(); i++)
    {
        if (isInterruptionRequested())
        {
            cancelled=true;
            break;
        }

        runner->results+="<hr><p>&raquo; Running test <strong>" + runner->testList.at(i)->getName() + "</strong> <span style=\"color: #7f7f7f; \">(" + runner->testList.at(i)->getDescription() + ")</span></p>";

        currentIndex=i;
        runner->testList.at(i)->run(runner->issues, runner->results);

        if (isInterruptionRequested())
        {
            cancelled=true;
            break;
        }
    }
    if (cancelled)
    {
        runner->results+="<hr><p style=\"color: #E5554F; \"><strong>Cancelled </strong></p>";
        runner->issues="<p>- Diagnostics incomplete -</p>";
    }
    else
    {
        runner->results+="<hr><p>Done at "+QDateTime::currentDateTime().toString()+"</p>";
    }

    if (runner->issues.isEmpty())
    {
        runner->issues="<p>No issues found</p>";
    }

    runner->isTerminating=false;
    runner->isActive=false;
    quit();
}


ydTestRunner::ydTestRunner() : testThread(this)
{
    results="";
    issues="";
    isActive=false;
}


bool ydTestRunner::runTests()
{
    if (isActive)
    {
        return false;
    }

    isTerminating=false;
    isActive=true;

    // Make sure that all test objects have a pointer to the broker instance
    for (int i=0; i<testList.count(); i++)
    {
        testList.at(i)->setBroker(&broker);
    }

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
    isTerminating=true;
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

