#ifndef YDTESTSYNGO_H
#define YDTESTSYNGO_H

#include "yd_test.h"

class ydTestSyngo : public ydTest
{
public:
    ydTestSyngo();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
};

#endif // YDTESTSYNGO_H
