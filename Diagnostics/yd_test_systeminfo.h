#ifndef YDTESTSYSTEMINFO_H
#define YDTESTSYSTEMINFO_H

#include "yd_test.h"

class ydTestSysteminfo : public ydTest
{
public:
    ydTestSysteminfo();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
};

#endif // YDTESTSYSTEMINFO_H
