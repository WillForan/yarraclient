#ifndef YDTESTYARRA_H
#define YDTESTYARRA_H

#include "yd_test.h"

class ydTestYarra : public ydTest
{
public:
    ydTestYarra();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
};


#endif // YDTESTYARRA_H
