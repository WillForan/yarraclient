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

    //broker->insert("result", "yes");

    return true;
}
