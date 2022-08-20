#ifndef YDTESTORT_H
#define YDTESTORT_H

#include "yd_test.h"
#include "../OfflineReconClient/ort_configuration.h"
#include "../OfflineReconClient/ort_serverlist.h"


class ydTestORT : public ydTest
{
public:
    ydTestORT();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
    void testConnectivity(QString& issues, QString& results);
    bool mountServerAndVerify(QString connectCmd, QString& issues, QString& results, bool isSeedServer, QString serverName="");


protected:
    ortConfiguration ortConfig;    
    ortServerList serverList;

};

#endif // YDTESTORT_H
