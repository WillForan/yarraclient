#ifndef YDTESTRDS_H
#define YDTESTRDS_H

#include "yd_test.h"
#include "../Client/rds_configuration.h"

class ydTestRDS : public ydTest
{
public:
    ydTestRDS();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);

protected:
    rdsConfiguration rdsConfig;

};

#endif // YDTESTRDS_H
