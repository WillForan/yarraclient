#include "yca_threadlog.h"
#include "yca_global.h"

ycaThreadLog* ycaThreadLog::pSingleton=0;


ycaThreadLog* ycaThreadLog::getInstance()
{
    if (pSingleton==0)
    {
        pSingleton=new ycaThreadLog();
    }
    return pSingleton;
}


ycaThreadLog::ycaThreadLog()
{
    logMutex.lock();

    QString logFilename=qApp->applicationDirPath()+"/log/yca.log";

    logfile.setFileName(logFilename);
    logfile.open(QIODevice::Append | QIODevice::Text);

    if (logfile.size() > YTL_MAXLOGSIZE)
    {
        log("Size of logfile getting large.");
        log("Renaming file and creating empty file.");
        logfile.flush();

        if (!logfile.rename(logFilename + "_" + QDate::currentDate().toString("ddMMyyHHmmss")))
        {
            // TODO: Error handling
        }

        logfile.close();
        logfile.setFileName(logFilename);
        logfile.open(QIODevice::Append | QIODevice::Text);
    }

    logfile.write(QString("---\n").toLatin1());
    QString startEntry=formatLine("Yarra Cloud Agent started (V "+QString(YCA_VERSION)+")", YTL_INFO, YTL_HIGH);
    logfile.write(startEntry.toLatin1());
    logfile.flush();

    logMutex.unlock();
}
