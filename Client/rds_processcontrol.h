#ifndef RDS_PROCESSCONTROL_H
#define RDS_PROCESSCONTROL_H

#include <QtGui>

#include "rds_global.h"


class rdsProcessControl
{
public:
    enum {
        STATE_IDLE           =0,
        STATE_RAIDTRANSFER   =1,
        STATE_NETWORKTRANSFER=2,
        STATE_NETWORKTRANSFER_ALTERNATING=3
    };

    rdsProcessControl();

    void setStartTime();

    int getState();
    bool isUpdateNeeded();
    void performUpdate();
    void requestPostponement();
    void setActivityWindow(bool showWindow);

    void setExplicitUpdate(int minutes);

    QDateTime getLastUpdateTime();
    bool isWaitingFirstUpdate();

    void sendScanInfoToLogServer();

protected:

    int state;

    bool explicitUpdate;
    QDateTime explicitUpdateTime;

    bool updateTime1Done;
    bool updateTime2Done;
    bool updateTime3Done;

    QDateTime startTime;
    QDateTime lastUpdate;
    QDateTime nextPeriodicUpdate;
    QDateTime lastCheckTime;

    bool      logServerOnlyUpdate;
    QDateTime lastLogServerOnlyUpdate;
    QDateTime nextLogServerOnlyUpdate;

    int connectionFailureCount;

    bool showActivityWindow;

    void setState(int newState);
    void setNextPeriodicUpdate();

};


inline int rdsProcessControl::getState()
{
    return state;
}


inline void rdsProcessControl::setActivityWindow(bool showWindow)
{
    showActivityWindow=showWindow;
}


inline void rdsProcessControl::setState(int newState)
{
    state=newState;
}


inline void rdsProcessControl::setExplicitUpdate(int minutes)
{
    explicitUpdate=true;
    explicitUpdateTime=QDateTime::currentDateTime();
    explicitUpdateTime=explicitUpdateTime.addSecs(minutes*60);
}


inline QDateTime rdsProcessControl::getLastUpdateTime()
{
    return lastUpdate;
}


inline bool rdsProcessControl::isWaitingFirstUpdate()
{
    return (startTime==lastUpdate);
}


#endif // RDS_PROCESSCONTROL_H

