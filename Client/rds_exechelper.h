#ifndef RDSEXECHELPER_H
#define RDSEXECHELPER_H

#include <QStringList>
#include <QProcess>


class rdsExecHelper : public QObject
{
    Q_OBJECT

public:
    rdsExecHelper();

    // These are generic functions to execute commands
    void setTimeout(int ms);
    void setCommand(QString& cmdLineToRun);

    bool run();
    bool run(QString cmdLine);
    QStringList output;

    static void safeSleep(int ms);


    // These two functions are only used for calling the "net use" command from ORT
    bool callNetUseTimout(int timeoutMs);
    void setMonitorNetUseOutput(bool state=true);


private:
    QProcess process;

    int      timeoutMs;
    QString  cmdLine;

    bool monitorNetUseOutput;
    bool detectedNetUseError;
    bool detectedNetUseSuccess;

public slots:
    void readNetUseOutput();

};


inline void rdsExecHelper::setTimeout(int ms)
{
    timeoutMs=ms;
}


inline void rdsExecHelper::setCommand(QString& cmdLineToRun)
{
    cmdLine=cmdLineToRun;
}


inline void rdsExecHelper::setMonitorNetUseOutput(bool state)
{
    monitorNetUseOutput=state;
}


#endif // RDSEXECHELPER_H
