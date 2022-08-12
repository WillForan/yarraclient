#include "yd_test_syngo.h"
#include "yd_global.h"


ydTestSyngo::ydTestSyngo() : ydTest()
{
}


QString ydTestSyngo::getName()
{
    return "Siemens Syngo";
}


QString ydTestSyngo::getDescription()
{
    return "indentify installed Siemens software";
}


QString ydTestSyngo::getIssues()
{
    return "";
}


QString ydTestSyngo::getResults()
{
    QString result = "Time is now "+QDateTime::currentDateTime().toString();
    result += "<p>"+YD_TEXT_ERROR("This should be in red!")+"</p>";
    result += "<p><ul>";
    result += "<li>"+YD_BADGE_ERROR("This is wrong")+"</li>";
    result += "<li>"+YD_BADGE_WARNING("This is wrong")+"</li>";
    result += "<li>"+YD_BADGE_SUCCESS("This is wrong")+"</li>";
    result += "<li>"+YD_BADGE_INFO("This is wrong")+"</li>";
    result += "</ul></p>";
    result += "Outcome: "+YD_TEXT_SUCCESS("OK");

    return result;
}


bool ydTestSyngo::run()
{
    QThread::msleep(100);

    return true;
}
