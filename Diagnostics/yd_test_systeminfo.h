#ifndef YDTESTSYSTEMINFO_H
#define YDTESTSYSTEMINFO_H

#include "yd_test.h"

class ydTestSysteminfo : public ydTest
{
public:
    ydTestSysteminfo();

    QString getName();
    QString getDescription();

    QString getIssuesHTML();
    QString getResultsHTML();
    QString getResultsText();

    bool run();
};

#endif // YDTESTSYSTEMINFO_H
