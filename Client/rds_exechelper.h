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

    QProcess process;
    QString execCmdLine;

public slots:
    void logOutput();

};




inline void rdsExecHelper::setCommand(QString& cmdLine)
{
    execCmdLine=cmdLine;
}


#endif // RDS_EXECHELPER_H
