#ifndef RDSEXECHELPER_H
#define RDSEXECHELPER_H

#include <QStringList>


class rdsExecHelper
{
public:
    rdsExecHelper();

    void setTimeout(int ms);
    bool run(QString cmdLine);

    QStringList output;

private:

    int timeoutMs;

};


inline void rdsExecHelper::setTimeout(int ms)
{
    timeoutMs=ms;
}


#endif // RDSEXECHELPER_H
