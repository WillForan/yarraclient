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
    bool run(QString cmdLine, QString nativeArguments="");
    QStringList output;

    static void safeSleep(int ms);


    // These two functions are only used for calling the "net use" command from ORT
    bool callNetUseTimout(int timeoutMs);
    void setMonitorNetUseOutput(bool state=true);
    QString getDetectedNetUseErrorMessage();

    int getExitCode();

private:
    QProcess process;

    int      timeoutMs;
    QString  cmdLine;
    int      exitCode;

    bool monitorNetUseOutput;
    bool detectedNetUseError;
    bool detectedNetUseSuccess;
    QString detectedNetUseErrorMessage;

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


inline int rdsExecHelper::getExitCode()
{
    return exitCode;
}

inline QString rdsExecHelper::getDetectedNetUseErrorMessage()
{
    return detectedNetUseErrorMessage;
}


#endif // RDSEXECHELPER_H
