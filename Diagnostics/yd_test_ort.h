#ifndef YDTESTORT_H
#define YDTESTORT_H

#include "yd_test.h"
#include "../OfflineReconClient/ort_configuration.h"


class ydTestORT : public ydTest
{
public:
    ydTestORT();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
    void testConnectivity(QString& issues, QString& results);


protected:
    ortConfiguration ortConfig;
};

#endif // YDTESTORT_H
