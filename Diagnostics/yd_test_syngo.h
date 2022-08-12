#ifndef YDTESTSYNGO_H
#define YDTESTSYNGO_H

#include "yd_test.h"

class ydTestSyngo : public ydTest
{
public:
    ydTestSyngo();

    QString getName();
    QString getDescription();

    QString getIssues();
    QString getResults();

    bool run();
};

#endif // YDTESTSYNGO_H
