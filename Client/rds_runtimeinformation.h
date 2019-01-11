#ifndef RDS_RUNTIMEINFORMATION_H
#define RDS_RUNTIMEINFORMATION_H

#include <QtGui>

#include "rds_log.h"
#include "rds_configuration.h"


class rdsRaid;
class rdsNetwork;
class rdsOperationWindow;
class rdsProcessControl;

class rdsRuntimeInformation
{
public:

    enum rdsModes
    {
        RDS_QUIT=0,
        RDS_CONFIGURATION,
        RDS_OPERATION
    };

    #define RDS_SYNGOVERSIONS_COUNT 18

    enum rdsSyngoVersions
    {
        RDS_INVALID = -1,
        RDS_VB13A   =  0,
        RDS_VB15A   =  1,
        RDS_VB17A   =  2,
        RDS_VD11A   =  3,
        RDS_VD11D   =  4,
        RDS_VD13A   =  5,
        RDS_VD13C   =  6,
        RDS_VB18P   =  7,
        RDS_VB19A   =  8,
        RDS_VB20P   =  9,
        RDS_VD13D   =  10,
        RDS_VE11A   =  11,
        RDS_VD13B   =  12,
        RDS_VE11B   =  13,
        RDS_VE11C   =  14,
        RDS_VE11U   =  15,
        RDS_VE11P   =  16,
        RDS_XA10A   =  17,
        RDS_VB19B   =  18,
        RDS_XA11A   =  19,
        RDS_VE12U   =  20
    };

    enum rdsSyngoLines
    {
        RDS_VB   = 0,
        RDS_VD   = 1,
        RDS_VE   = 2,
        RDS_XA   = 3
    };

    enum rdsRaidToolFormat
    {
        RDS_RAIDTOOL_VB    = 0,
        RDS_RAIDTOOL_VD11  = 1,
        RDS_RAIDTOOL_VD13C = 2,
        RDS_RAIDTOOL_VE    = 3,
        RDS_RAIDTOOL_VB15  = 4
    };


    static rdsRuntimeInformation* getInstance();

    void setMode(rdsModes newMode);
    rdsModes getMode();

    int getReturnValue();
    void setReturnValue(int value);

    void prepare();
    bool prepareEnvironment();

    bool isSimulation();
    bool isInvalidEnvironment();

    QString getAppPath();

    void setLogInstance(rdsLog* instance);
    void flushLog();
    void log  (QString text);
    void debug(QString text);
    rdsLog* getLogInstance();

    void setConfigInstance(rdsConfiguration* instance);
    rdsConfiguration* getConfigInstance();

    void setRaidInstance(rdsRaid* instance);
    rdsRaid* getRaidInstance();

    void setNetworkInstance(rdsNetwork* instance);
    rdsNetwork* getNetworkInstance();

    void setControlInstance(rdsProcessControl* instance);
    rdsProcessControl* getControlInstance();

    void setWindowInstance(rdsOperationWindow* instance);
    rdsOperationWindow* getWindowInstance();

    int     getSyngoMRVersion();
    QString getSyngoMRVersionString(int syngoVersionEnum=RDS_INVALID);
    int     getSyngoMRLine();

    bool    isSyngoVBLine();
    bool    isSyngoVDLine();
    bool    isSyngoVELine();
    bool    isSyngoXALine();

    QString getSyngoImagerIP();
    int     getRaidToolFormat();
    QString getSyngoXABinPath();

    qint64 getFreeDiskSpace(QString path="");

    bool isDebugMode();
    void setDebugMode(bool enabled);

    void processEvents();

    void setPostponementRequest(bool request);
    bool isPostponementRequested();

    void setSevereErrors(bool value);
    bool isSevereErrors();

    void setPreventStart();

    void showOperationWindow();
    void clearLogWidget();
    void updateInfoUI();

    void setIconWindowAnim(bool status);

    void debugPatchSyngoVersion(int newVersion);

private:    

    rdsRuntimeInformation();
    static rdsRuntimeInformation* pSingleton;

    rdsModes mode;
    int returnValue;
    bool simulation;
    bool invalidEnvironment;

    bool simulatorExists;
    bool syngoMRExists;
    bool debugMode;

    bool preventStart;

    QString appPath;
    rdsLog* logInstance;
    rdsConfiguration* configInstance;
    rdsRaid* raidInstance;
    rdsNetwork* networkInstance;
    rdsOperationWindow* windowInstance;
    rdsProcessControl* controlInstance;

    int syngoMRVersion;
    int syngoMRLine;

    void determineSyngoVersion();
    int  determineNumarisXVersion();

    bool postponementRequest;
    bool severeErrors;

    bool    detectedNumarisX;
    QString numarisXBinPath;
};


inline void rdsRuntimeInformation::setMode(rdsRuntimeInformation::rdsModes newMode)
{
    mode=newMode;
}


inline rdsRuntimeInformation::rdsModes rdsRuntimeInformation::getMode()
{
    return mode;
}


inline int rdsRuntimeInformation::getReturnValue()
{
    return returnValue;
}


inline void rdsRuntimeInformation::setReturnValue(int value)
{
    returnValue=value;
}


inline bool rdsRuntimeInformation::isSimulation()
{
    return simulation;
}


inline bool rdsRuntimeInformation::isInvalidEnvironment()
{
    return invalidEnvironment;
}


inline QString rdsRuntimeInformation::getAppPath()
{
    return appPath;
}


inline void rdsRuntimeInformation::setLogInstance(rdsLog* instance)
{
    logInstance=instance;
}


inline rdsLog* rdsRuntimeInformation::getLogInstance()
{
    return logInstance;
}


inline void rdsRuntimeInformation::log(QString text)
{
    if (logInstance!=0)
    {
        logInstance->log(text);
    }
}


inline void rdsRuntimeInformation::debug(QString text)
{
    if (!debugMode)
    {
        return;
    }

    if (logInstance!=0)
    {
        logInstance->debug(text);
    }
}


inline void rdsRuntimeInformation::flushLog()
{
    if (logInstance!=0)
    {
        logInstance->flush();
    }
}


inline int rdsRuntimeInformation::getSyngoMRVersion()
{
    return syngoMRVersion;
}


inline void rdsRuntimeInformation::setConfigInstance(rdsConfiguration* instance)
{
    configInstance=instance;
}


inline rdsConfiguration* rdsRuntimeInformation::getConfigInstance()
{
    return configInstance;
}


inline void rdsRuntimeInformation::setRaidInstance(rdsRaid* instance)
{
    raidInstance=instance;
}


inline rdsRaid* rdsRuntimeInformation::getRaidInstance()
{
    return raidInstance;
}


inline void rdsRuntimeInformation::setNetworkInstance(rdsNetwork* instance)
{
    networkInstance=instance;
}


inline rdsNetwork* rdsRuntimeInformation::getNetworkInstance()
{
    return networkInstance;
}


inline void rdsRuntimeInformation::setControlInstance(rdsProcessControl* instance)
{
    controlInstance=instance;
}


inline rdsProcessControl* rdsRuntimeInformation::getControlInstance()
{
    return controlInstance;
}


inline void rdsRuntimeInformation::setWindowInstance(rdsOperationWindow* instance)
{
    windowInstance=instance;
}


inline rdsOperationWindow* rdsRuntimeInformation::getWindowInstance()
{
    return windowInstance;
}


inline QString rdsRuntimeInformation::getSyngoMRVersionString(int syngoVersionEnum)
{
    QString versionString="";

    // If the function is called without parameter, return the syngo version
    // detected by the RTI
    if (syngoVersionEnum==RDS_INVALID)
    {
        syngoVersionEnum=syngoMRVersion;
    }

    switch (syngoVersionEnum)
    {
    case RDS_VB13A:
        versionString="VB13A";
        break;

    case RDS_VB15A:
        versionString="VB15A";
        break;

    case RDS_VB17A:
        versionString="VB17A";
        break;

    case RDS_VB19A:
        versionString="VB19A";
        break;

    case RDS_VB18P:
        versionString="VB18P";
        break;

    case RDS_VB20P:
        versionString="VB20P";
        break;

    case RDS_VD11A:
        versionString="VD11A";
        break;

    case RDS_VD11D:
        versionString="VD11D";
        break;

    case RDS_VD13A:
        versionString="VD13A";
        break;

    case RDS_VD13B:
        versionString="VD13B";
        break;

    case RDS_VD13C:
        versionString="VD13C";
        break;

    case RDS_VD13D:
        versionString="VD13D";
        break;

    case RDS_VE11A:
        versionString="VE11A";
        break;

    case RDS_VE11B:
        versionString="VE11B";
        break;

    case RDS_VE11C:
        versionString="VE11C";
        break;

    case RDS_VE11U:
        versionString="VE11U";
        break;

    case RDS_VE11P:
        versionString="VE11P";
        break;

    case RDS_XA10A:
        versionString="XA10A";
        break;

    case RDS_VB19B:
        versionString="VB19B";
        break;

    case RDS_XA11A:
        versionString="XA11A";
        break;

    case RDS_VE12U:
        versionString="VE12U";
        break;

    default:
        versionString="Unknown - Use with care!";
        break;
    }

    return versionString;
}


inline bool rdsRuntimeInformation::isSyngoVBLine()
{
    return (syngoMRLine==RDS_VB);
}


inline bool rdsRuntimeInformation::isSyngoVDLine()
{
    return (syngoMRLine==RDS_VD);
}


inline bool rdsRuntimeInformation::isSyngoVELine()
{
    return (syngoMRLine==RDS_VE);
}


inline bool rdsRuntimeInformation::isSyngoXALine()
{
    return (syngoMRLine==RDS_XA);
}


inline bool rdsRuntimeInformation::isDebugMode()
{
    return debugMode;
}


inline void rdsRuntimeInformation::setDebugMode(bool enabled)
{
    debugMode=enabled;
}


inline void rdsRuntimeInformation::processEvents()
{
    QCoreApplication::processEvents();
}


inline void rdsRuntimeInformation::setPostponementRequest(bool request)
{
    postponementRequest=request;
}


inline bool rdsRuntimeInformation::isPostponementRequested()
{
    return postponementRequest;
}


inline void rdsRuntimeInformation::setSevereErrors(bool value)
{
    severeErrors=value;
}


inline bool rdsRuntimeInformation::isSevereErrors()
{
    return severeErrors;
}


inline int rdsRuntimeInformation::getRaidToolFormat()
{
    switch (syngoMRVersion)
    {
    case RDS_VD11A:
    case RDS_VD11D:
    case RDS_VD13A:
    case RDS_VD13B:
        return RDS_RAIDTOOL_VD11;
        break;
    case RDS_VD13C:
    case RDS_VD13D:
        return RDS_RAIDTOOL_VD13C;
        break;
    case RDS_VE11A:
    case RDS_VE11B:
    case RDS_VE11C:
    case RDS_VE11U:
    case RDS_VE11P:
    case RDS_XA10A:
    case RDS_XA11A:
    case RDS_VE12U:
        return RDS_RAIDTOOL_VE;
        break;
    case RDS_VB15A:
    case RDS_VB13A:
        return RDS_RAIDTOOL_VB15;
        break;
    default:
        return RDS_RAIDTOOL_VB;
    };
}


inline int rdsRuntimeInformation::getSyngoMRLine()
{
    switch (syngoMRVersion)
    {
    case RDS_VB13A:
    case RDS_VB15A:
    case RDS_VB17A:
    case RDS_VB19A:
    case RDS_VB18P:
    case RDS_VB20P:
    case RDS_VB19B:
    default:
        return RDS_VB;
        break;
    case RDS_VD11A:
    case RDS_VD11D:
    case RDS_VD13A:
    case RDS_VD13B:
    case RDS_VD13C:
    case RDS_VD13D:
        return RDS_VD;
        break;
    case RDS_VE11A:
    case RDS_VE11B:
    case RDS_VE11C:
    case RDS_VE11U:
    case RDS_VE11P:
    case RDS_VE12U:
        return RDS_VE;
        break;
    case RDS_XA10A:
    case RDS_XA11A:
        return RDS_XA;
        break;
    }
}


inline void rdsRuntimeInformation::setPreventStart()
{
    preventStart=true;
}


inline QString rdsRuntimeInformation::getSyngoXABinPath()
{
    return numarisXBinPath;
}


#endif // RDS_RUNTIMEINFORMATION_H

