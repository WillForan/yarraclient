#ifndef RDS_EXECHELPER_H
#define RDS_EXECHELPER_H

#include <QtCore>


class rdsExecHelper : public QObject
{
    Q_OBJECT

public:
    rdsExecHelper();

    void setCommand(QString& cmdLine);
    bool callProcessTimout(int timeoutMs);

    void setMonitorOutput(bool state=true);

    QProcess process;
    QString execCmdLine;

    static void safeSleep(int ms);

private:
    bool monitorOutput;

    bool detectedError;
    bool detectedSuccess;

public slots:
    void readOutput();

};



inline void rdsExecHelper::setCommand(QString& cmdLine)
{
    execCmdLine=cmdLine;
}


inline void rdsExecHelper::setMonitorOutput(bool state)
{
    monitorOutput=state;
}



#endif // RDS_EXECHELPER_H

