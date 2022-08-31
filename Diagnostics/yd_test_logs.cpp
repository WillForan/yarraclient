#include "yd_test_logs.h"
#include "yd_global.h"


ydTestLogs::ydTestLogs() : ydTest()
{
}


QString ydTestLogs::getName()
{
    return "Logs";
}


QString ydTestLogs::getDescription()
{
    return "captures latest log output";
}


bool ydTestLogs::run(QString& issues, QString& results)
{
    YD_RESULT_STARTSECTION           
    YD_ADDRESULT("Including latest log entries...");

    QDir logPath(qApp->applicationDirPath());
    if (!logPath.cd("log"))
    {
        YD_ADDRESULT_COLORLINE("Yarra log folder not found", YD_CRITICAL);
        YD_ADDISSUE("Yarra log folder not found", YD_CRITICAL);
        YD_RESULT_ENDSECTION
        return true;
    }
    YD_RESULT_ENDSECTION

    YD_RESULT_STARTSECTION
    YD_ADDRESULT("<u>ORT log (100 last lines):</u>");
    if (!logPath.exists("ort.log"))
    {
        YD_ADDRESULT_COLORLINE("ORT log file not found", YD_INFO);
    }
    else
    {
        QFile logFile(logPath.absoluteFilePath("ort.log"));
        if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            YD_ADDRESULT_COLORLINE("Unable to open ORT log", YD_CRITICAL);
            YD_ADDISSUE("Unable to open ORT log", YD_CRITICAL);
        }
        else
        {
            QStringList logContent;
            QTextStream inStream(&logFile);
            while (!inStream.atEnd())
            {
                QString line = inStream.readLine();
                logContent.append(line);
            }

            bool foundErrors=false;

            YD_ADDRESULT_LINE("<div style=\"font-family: monospace; color: #CCC; background-color: #141414; \">");
            int endLine = logContent.count();
            int startLine = logContent.count() - 100;
            if (startLine < 0)
            {
                startLine = 0;
            }
            for (int i=startLine; i<endLine; i++)
            {
                YD_ADDRESULT_LINE(logContent.at(i));
                if (logContent.at(i).contains("ERROR:", Qt::CaseInsensitive))
                {
                    foundErrors=true;
                }
            }
            YD_ADDRESULT_LINE("</div>");

            if (foundErrors)
            {
                YD_ADDRESULT_COLORLINE("Errors found in ORT log", YD_WARNING);
                YD_ADDISSUE("Errors found in ORT log", YD_WARNING);
            }
        }
    }
    YD_RESULT_ENDSECTION

    YD_RESULT_STARTSECTION
    YD_ADDRESULT("<u>RDS log (100 last lines):</u>");
    if (!logPath.exists("rds.log"))
    {
        YD_ADDRESULT_COLORLINE("RDS log file not found", YD_INFO);
    }
    else
    {
        QFile logFile(logPath.absoluteFilePath("rds.log"));
        if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            YD_ADDRESULT_COLORLINE("Unable to open RDS log", YD_CRITICAL);
            YD_ADDISSUE("Unable to open RDS log", YD_CRITICAL);
        }
        else
        {
            QStringList logContent;
            QTextStream inStream(&logFile);
            while (!inStream.atEnd())
            {
                QString line = inStream.readLine();
                logContent.append(line);
            }

            bool foundErrors=false;

            YD_ADDRESULT_LINE("<div style=\"font-family: monospace; color: #CCC; background-color: #141414; \">");
            int endLine = logContent.count();
            int startLine = logContent.count() - 100;
            if (startLine < 0)
            {
                startLine = 0;
            }
            for (int i=startLine; i<endLine; i++)
            {
                YD_ADDRESULT_LINE(logContent.at(i));
                if (logContent.at(i).contains("ERROR:", Qt::CaseInsensitive))
                {
                    foundErrors=true;
                }

            }
            YD_ADDRESULT_LINE("</div>");

            if (foundErrors)
            {
                YD_ADDRESULT_COLORLINE("Errors found in RDS log", YD_WARNING);
                YD_ADDISSUE("Errors found in RDS log", YD_WARNING);
            }

        }
    }
    YD_RESULT_ENDSECTION

    return true;
}
