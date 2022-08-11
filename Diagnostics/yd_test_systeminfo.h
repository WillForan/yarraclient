#ifndef YDTESTSYSTEMINFO_H
#define YDTESTSYSTEMINFO_H

#include "yd_test.h"

class ydTestSysteminfo : public ydTest
{
public:
    ydTestSysteminfo();

    QString getName();
    QString getDescription();

    QString getIssues();
    QString getResults();

    bool run();
};

#endif // YDTESTSYSTEMINFO_H
