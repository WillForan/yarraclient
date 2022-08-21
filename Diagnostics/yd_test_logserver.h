#ifndef YDTESTLOGSERVER_H
#define YDTESTLOGSERVER_H

#include "yd_test.h"

class ydTestLogServer : public ydTest
{
public:
    ydTestLogServer();

    QString getName();
    QString getDescription();

    bool run(QString& issues, QString& results);
    void testConnection(QString& issues, QString& results);

protected:
    QString logServerAddress;
    QString logServerAPIKey;

};

#endif // YDTESTLOGSERVER_H
