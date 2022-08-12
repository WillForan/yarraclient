#include "yd_test_syngo.h"
#include "yd_global.h"


ydTestSyngo::ydTestSyngo() : ydTest()
{
}


QString ydTestSyngo::getName()
{
    return "Syngo";
}


QString ydTestSyngo::getDescription()
{
    return "indentify installed Siemens software";
}


bool ydTestSyngo::run(QString& issues, QString& results)
{
    results += "<p>Time is now "+QDateTime::currentDateTime().toString()+"</p>";
    QThread::msleep(1000);
    results += "<p>"+YD_TEXT_ERROR("This should be in red!")+"</p>";
    QThread::msleep(1000);
    results += "<p><ul>";
    results += "<li>"+YD_BADGE_ERROR("This is wrong")+"</li>";
    results += "<li>"+YD_BADGE_WARNING("This is wrong")+"</li>";
    results += "<li>"+YD_BADGE_SUCCESS("This is wrong")+"</li>";
    results += "<li>"+YD_BADGE_INFO("This is wrong")+"</li>";
    results += "</ul></p>";
    QThread::msleep(1000);
    results += "<p>Outcome: "+YD_TEXT_SUCCESS("OK")+"</p>";

    if (!broker->contains("result"))
    {
        YD_ADDISSUE("Value not in broker!", YD_CRITICAL)
        YD_ADDISSUE("Diskspace is low", YD_WARNING)
    }

    return true;
}
