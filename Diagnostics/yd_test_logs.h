#ifndef YDTESTLOGS_H
#define YDTESTLOGS_H

#include "yd_test.h"

class ydTestLogs : public ydTest
{
public:
    ydTestLogs();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
};

#endif // YDTESTLOGS_H
