#include "rds_runtimeinformation.h"

#include <QtWidgets>
#include "rds_global.h"
#include "rds_operationwindow.h"

#include <windows.h>

rdsRuntimeInformation* rdsRuntimeInformation::pSingleton=0;


rdsRuntimeInformation::rdsRuntimeInformation()
{
    mode=RDS_OPERATION;
    returnValue=0;
    simulation=false;
    invalidEnvironment=false;
    preventStart=false;

    simulatorExists=false;
    syngoMRExists=false;
    appPath="";
    logInstance=0;
    configInstance=0;
    raidInstance=0;
    networkInstance=0;
    windowInstance=0;
    controlInstance=0;

    debugMode=false;

    syngoMRVersion=RDS_INVALID;

    postponementRequest=false;
    severeErrors=false;
}


rdsRuntimeInformation* rdsRuntimeInformation::getInstance()
{
    if (pSingleton==0)
    {
        pSingleton=new rdsRuntimeInformation();
    }
    return pSingleton;
}


void rdsRuntimeInformation::prepare()
{
    // Check for existance of RaidSimulator
    simulatorExists=false;
    QString myPath=qApp->applicationDirPath();
    QDir myDir(myPath);
    if (myDir.exists(RDS_RAIDSIMULATOR_NAME))
    {
        simulatorExists=true;
    }

    // Check for existance of syngoMR
    QDir syngoDir(RDS_RAIDTOOL_PATH);
    syngoMRExists=false;

    if (syngoDir.exists())
    {
        if (syngoDir.exists(RDS_RAIDTOOL_PATH))
        {
            syngoMRExists=true;
        }
    }

    // Check syngo Version
    if (syngoMRExists)
    {
        determineSyngoVersion();
    }

    // Check if we have a valid environment
    if ((!simulatorExists) && (!syngoMRExists))
    {
        invalidEnvironment=true;
    }
    else
    {
        invalidEnvironment=false;
    }

    simulation=true;

    if (syngoMRExists)
    {
        simulation=false;
    }

    if (simulation)
    {
        syngoMRVersion=RDS_VB17A;
        syngoMRLine=RDS_VB;
    }

    // Initialize random generator for jittering start times
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());
}


bool rdsRuntimeInformation::prepareEnvironment()
{
    // If any prior constructors request that start of the
    // client is prevented
    if (preventStart)
    {
        return false;
    }


    bool error=false;

    // Prepare all directories etc

    // Change directory to application path
    appPath=qApp->applicationDirPath();
    QDir dir;
    dir.setCurrent(appPath);

    if (appPath.contains(" "))
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Invalid Installation Path");
        msgBox.setText("The RDS tool has been installed in an invalid path. The path must not contain any space characters. Please move the program to a different location.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(RDS_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        return false;
    }

    // Check if directory for logfiles exists
    if (!dir.exists(RDS_DIR_LOG))
    {
        // If not, create it
        dir.mkdir(RDS_DIR_LOG);
    }

    // Check if it has been created successfully
    if (!dir.exists(RDS_DIR_LOG))
    {
        error=true;
    }

    // Check if directory for queued files
    if (!dir.exists(RDS_DIR_QUEUE))
    {
        dir.mkdir(RDS_DIR_QUEUE);
    }

    if (!dir.exists(RDS_DIR_QUEUE))
    {
        error=true;
    }


    // Show error message because the log is not running yet
    if (error)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText("Unable to create required directories.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(RDS_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }

    return (!error);
}


#define RDS_SYNGODETECT(ID,IDFILE) if (myDir.exists(IDFILE)) { syngoMRVersion=ID; }


void rdsRuntimeInformation::determineSyngoVersion()
{
    // Detect the syngo MR version by searching for existance of version files
    syngoMRVersion=RDS_INVALID;
    QDir myDir;  

    // VB line
    RDS_SYNGODETECT(RDS_VB13A,RDS_SYNGODETECT_VB13A);
    RDS_SYNGODETECT(RDS_VB15A,RDS_SYNGODETECT_VB15A);
    RDS_SYNGODETECT(RDS_VB17A,RDS_SYNGODETECT_VB17A);
    RDS_SYNGODETECT(RDS_VB19A,RDS_SYNGODETECT_VB19A);
    // VB-P line
    RDS_SYNGODETECT(RDS_VB18P,RDS_SYNGODETECT_VB18P);
    RDS_SYNGODETECT(RDS_VB20P,RDS_SYNGODETECT_VB20P);
    // VD line
    RDS_SYNGODETECT(RDS_VD11A,RDS_SYNGODETECT_VD11A);
    RDS_SYNGODETECT(RDS_VD11D,RDS_SYNGODETECT_VD11D);
    RDS_SYNGODETECT(RDS_VD13A,RDS_SYNGODETECT_VD13A);
    RDS_SYNGODETECT(RDS_VD13B,RDS_SYNGODETECT_VD13B);
    RDS_SYNGODETECT(RDS_VD13C,RDS_SYNGODETECT_VD13C);
    RDS_SYNGODETECT(RDS_VD13D,RDS_SYNGODETECT_VD13D);
    // VE line
    RDS_SYNGODETECT(RDS_VE11A,RDS_SYNGODETECT_VE11A);
    RDS_SYNGODETECT(RDS_VE11B,RDS_SYNGODETECT_VE11B);
    RDS_SYNGODETECT(RDS_VE11C,RDS_SYNGODETECT_VE11C);
    RDS_SYNGODETECT(RDS_VE11U,RDS_SYNGODETECT_VE11U);
    RDS_SYNGODETECT(RDS_VE11P,RDS_SYNGODETECT_VE11P);

    switch (syngoMRVersion)
    {
    case RDS_VB13A:
    case RDS_VB15A:
    case RDS_VB17A:        
    case RDS_VB19A:
    case RDS_VB18P:
    case RDS_VB20P:
        syngoMRLine=RDS_VB;
        break;
    case RDS_VD11A:
    case RDS_VD11D:
    case RDS_VD13A:
    case RDS_VD13B:
    case RDS_VD13C:
    case RDS_VD13D:
        syngoMRLine=RDS_VD;
        break;
    case RDS_VE11A:
    case RDS_VE11B:
    case RDS_VE11C:
    case RDS_VE11U:
    case RDS_VE11P:
        syngoMRLine=RDS_VE;
        break;
    }
}


QString rdsRuntimeInformation::getSyngoImagerIP()
{
    QString result=RDS_IMAGER_IP_3;

    if ((syngoMRVersion==RDS_VD13A) || (syngoMRVersion==RDS_VD13B) ||
        (syngoMRVersion==RDS_VD13C) || (syngoMRVersion==RDS_VD13D) ||
        (syngoMRLine==RDS_VE))
    {
        result=RDS_IMAGER_IP_2;
    }

    return result;
}


qint64 rdsRuntimeInformation::getFreeDiskSpace(QString path)
{
    // If no parameter is given, then test the disk space of the
    // application path, i.e. the queueing directory
    if (path=="")
    {
        path=appPath;
    }

    QString oldDir = QDir::current().absolutePath();
    QDir::setCurrent(path);

    ULARGE_INTEGER freeSpace,totalSpace;
    bool bRes = ::GetDiskFreeSpaceExA( 0 , &freeSpace , &totalSpace , NULL );
    if ( !bRes )
    {
        return 0;
    }

    QDir::setCurrent( oldDir );

    return static_cast<__int64>(freeSpace.QuadPart);
}


void rdsRuntimeInformation::showOperationWindow()
{
    // Exclude UI related stuff when compiling for the ORT app.
    #ifdef YARRA_APP_RDS

    if (windowInstance==0)
    {
        return;
    }

    windowInstance->showNormal();

    #endif
}


void rdsRuntimeInformation::clearLogWidget()
{
    #ifdef YARRA_APP_RDS

    if (logInstance!=0)
    {
        logInstance->clearLogWidget();
    }

    #endif
}


void rdsRuntimeInformation::updateInfoUI()
{
    #ifdef YARRA_APP_RDS

    if (windowInstance!=0)
    {
        windowInstance->updateInfoUI();
    }

    #endif
}


void rdsRuntimeInformation::setIconWindowAnim(bool status)
{
    #ifdef YARRA_APP_RDS

    if (configInstance->infoShowIcon)
    {
        windowInstance->iconWindow.setAnim(status);
    }

    #endif
}

