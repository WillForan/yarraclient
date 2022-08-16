#ifndef YDTESTRDS_H
#define YDTESTRDS_H

#include "yd_test.h"

class ydTestRDS : public ydTest
{
public:
    ydTestRDS();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
};

#endif // YDTESTRDS_H
