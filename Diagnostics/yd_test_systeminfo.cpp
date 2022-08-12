#include "yd_test_systeminfo.h"

#include "yd_global.h"


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


bool ydTestSysteminfo::run()
{
    QThread::msleep(100);

    return true;
}
