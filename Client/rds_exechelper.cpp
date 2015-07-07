#include "rds_exechelper.h"
#include "rds_global.h"

#if defined(Q_OS_WIN)
    #include <QtCore/QLibrary>
    #include <QtCore/qt_windows.h>
#endif
#if defined(Q_OS_UNIX)
    #include <time.h>
#endif

#include <QtWidgets>

rdsExecHelper::rdsExecHelper()
{
    execCmdLine="";
    process.setReadChannel(QProcess::StandardOutput);
    process.setProcessChannelMode(QProcess::MergedChannels);
}


bool rdsExecHelper::callProcessTimout(int timeoutMs)
{
    bool success=false;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(timeoutMs);
    QEventLoop q;
    connect(&process, SIGNAL(finished(int , QProcess::ExitStatus)), &q, SLOT(quit()));
    connect(&process, SIGNAL(error(QProcess::ProcessError)), &q, SLOT(quit()));
    connect(&timeoutTimer, SIGNAL(timeout()), &q, SLOT(quit()));
    connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(logOutput()));

    // Time measurement to diagnose RaidTool calling problems
    QTime ti;
    ti.start();
    timeoutTimer.start();

    // Start the process. Note: The commandline and arguments need to be defined before.
    process.start(execCmdLine);
    q.exec();

    // Check for problems with the event loop: Sometimes it seems to return to quickly!
    // In this case, start a second while loop to check when the process is really finished.
    if ((timeoutTimer.isActive()) && (process.state()==QProcess::Running))
    {
        timeoutTimer.stop();
        RTI->log("Warning: QEventLoop returned too early. Starting secondary loop.");
        while ((process.state()==QProcess::Running) && (ti.elapsed()<timeoutMs))
        {
            RTI->processEvents();
            Sleep(RDS_SLEEP_INTERVAL);
        }

        // If the process did not finish within the timeout duration
        if (process.state()==QProcess::Running)
        {
            RTI->log("Warning: Process is still active. Killing process.");
            process.kill();
            success=false;
        }
        else
        {
            success=true;
        }
    }
    else
    {
        // Normal timeout-handling if QEventLoop works normally
        if (timeoutTimer.isActive())
        {
            success=true;
            timeoutTimer.stop();
        }
        else
        {
            RTI->log("Warning: Process event loop timed out.");
            RTI->log("Warning: Duration since start "+QString::number(ti.elapsed())+" ms");
            success=false;
            if (process.state()==QProcess::Running)
            {
                RTI->log("Warning: Process is still active. Killing process.");
                process.kill();
            }
        }
    }

    logOutput();

    return success;
}


void rdsExecHelper::logOutput()
{
    while (process.canReadLine())
    {
        // Read the current line, but restrict the maximum length to 512 chars to
        // avoid infinite output (if a module starts outputting binary data)
        RTI->log(process.readLine(512));
    }
}
