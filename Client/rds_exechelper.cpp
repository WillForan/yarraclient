#include <QProcess>
#include <QTimer>

#include "rds_exechelper.h"
#include "rds_global.h"


rdsExecHelper::rdsExecHelper()
{
    timeoutMs=RDS_PROC_TIMEOUT;
    output.clear();
}


bool rdsExecHelper::run(QString cmdLine)
{
    // Clear the output buffer
    output.clear();

    QProcess *myProcess=new QProcess(0);
    myProcess->setReadChannel(QProcess::StandardOutput);

    bool success=false;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(timeoutMs);
    QEventLoop q;
    QObject::connect(myProcess, SIGNAL(finished(int , QProcess::ExitStatus)), &q, SLOT(quit()));
    QObject::connect(&timeoutTimer, SIGNAL(timeout()), &q, SLOT(quit()));

    // Time measurement to diagnose RaidTool calling problems
    QTime ti;
    ti.start();
    timeoutTimer.start();
    myProcess->start(cmdLine);
    q.exec();

    // Check for problems with the event loop: Sometimes it seems to return to quickly!
    // In this case, start a second while loop to check when the process is really finished.
    if ((timeoutTimer.isActive()) && (myProcess->state()==QProcess::Running))
    {
        timeoutTimer.stop();
        RTI->log("Warning: QEventLoop returned too early. Starting secondary loop.");
        while ((myProcess->state()==QProcess::Running) && (ti.elapsed()<timeoutMs))
        {
            RTI->processEvents();
            Sleep(RDS_SLEEP_INTERVAL);
        }

        // If the process did not finish within the timeout duration
        if (myProcess->state()==QProcess::Running)
        {
            RTI->log("Warning: Process is still active. Killing process.");
            myProcess->kill();
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
            if (myProcess->state()==QProcess::Running)
            {
                RTI->log("Warning: Process is still active. Killing process.");
                myProcess->kill();
            }
        }
    }

    if (!success)
    {
        // The process timeed out. Probably some error occured.
        RTI->log("ERROR: Timeout during cmd execution!");
    }
    else
    {
        // Read the output lines from the process and store in string list
        char buf[1024];
        qint64 lineLength = -1;

        do
        {
            lineLength=myProcess->readLine(buf, sizeof(buf));
            if (lineLength != -1)
            {
                output << QString(buf);
            }
        } while (lineLength!=-1);
    }

    delete myProcess;
    myProcess=0;

    return success;
}
