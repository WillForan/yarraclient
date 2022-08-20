#include "yd_helper.h"
#include "../Client/rds_exechelper.h"


ydHelper::ydHelper()
{
}


bool ydHelper::pingServer(QString ipAddress)
{
    if (ipAddress.isEmpty())
    {
        return false;
    }

    QString cmd="ping " + ipAddress + " -n 1";
    rdsExecHelper execHelper;
    execHelper.setCommand(cmd);
    execHelper.run();

    if (execHelper.getExitCode()!=0)
    {
        return false;
    }
    else
    {
        return true;
    }
}


QString ydHelper::extractIP(QString netUseCommand)
{
    if (netUseCommand.contains("net use"))
    {
        if (netUseCommand.contains("\\\\"))
        {
            netUseCommand.remove(0, netUseCommand.indexOf("\\\\")+2);
        }
        if (netUseCommand.contains("\\"))
        {
            netUseCommand.remove(netUseCommand.indexOf("\\"), netUseCommand.length());
            return netUseCommand;
        }
    }

    return "";
}

