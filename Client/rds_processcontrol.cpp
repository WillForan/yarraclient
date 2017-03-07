#include "rds_processcontrol.h"
#include "rds_global.h"
#include "rds_activitywindow.h"
#include "rds_raid.h"
#include "rds_network.h"
#include <QXmlStreamWriter>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

rdsProcessControl::rdsProcessControl()
{
    explicitUpdate=false;
    updateTime1Done=false;
    updateTime2Done=false;
    updateTime3Done=false;
    state=STATE_IDLE;
    connectionFailureCount=0;
    showActivityWindow=true;
}


bool rdsProcessControl::isUpdateNeeded()
{
    if (state!=STATE_IDLE)
    {
        RTI->log("Warning: Received update request in non-idle state");
        return false;
    }

    // Check update conditions!
    bool updateNeeded=false;
    QDateTime currTime = QDateTime::currentDateTime();

    if ((explicitUpdate) && (currTime>explicitUpdateTime))
    {
        updateNeeded=true;
        RTI->debug("Update condition: Explicit");
    }

    if (RTI_CONFIG->infoUpdateMode==RTI_CONFIG->UPDATEMODE_PERIODIC)
    {
        if (currTime>nextPeriodicUpdate)
        {
            updateNeeded=true;
            RTI->debug("Update condition: Periodic");
        }
    }

    if (currTime.date()>lastCheckTime.date())
    {
        // It's a new day. Reset the update status for the fixed time modus
        updateTime1Done=false;
        updateTime2Done=false;
        updateTime3Done=false;
        RTI->debug("Update check: New day detected");
        RTI->clearLogWidget();
    }

    if ((RTI_CONFIG->infoUpdateMode==RTI_CONFIG->UPDATEMODE_FIXEDTIME) ||
        (RTI_CONFIG->infoUpdateMode==RTI_CONFIG->UPDATEMODE_STARTUP_FIXEDTIME) )
    {
        if ((!updateTime1Done) && (currTime.time() > RTI_CONFIG->infoUpdateTime1))
        {
            updateTime1Done=true;
            updateNeeded=true;
            RTI->debug("Update condition: Fixed Time 1");
        }

        if ((RTI_CONFIG->infoUpdateUseTime2) && (!updateTime2Done) && (currTime.time() > RTI_CONFIG->infoUpdateTime2))
        {
            updateTime2Done=true;
            updateNeeded=true;
            RTI->debug("Update condition: Fixed Time 2");
        }

        if ((RTI_CONFIG->infoUpdateUseTime3) && (!updateTime3Done) && (currTime.time() > RTI_CONFIG->infoUpdateTime3))
        {
            updateTime3Done=true;
            updateNeeded=true;
            RTI->debug("Update condition: Fixed Time 3");
        }
    }

    lastCheckTime=currTime;

    return updateNeeded;
}


void rdsProcessControl::performUpdate()
{
    RTI->setSevereErrors(false);
    explicitUpdate=false;
    bool alternatingUpdate=false;
    qint64 diskSpace=RTI->getFreeDiskSpace();
    RTI->debug("Disk space on appdir = " + QString::number(diskSpace));

    RTI_NETLOG->postEvent(EventInfo::Type::Update,EventInfo::Detail::Start,EventInfo::Severity::Success,"Performing an update");
    // Use the alternating update mode when the disk space is below 5 Gb.
    // In the alternating mode, files are fetched from RAID and transferred
    // to the storage individually, which leads to longer dead time of the
    // scanner

    // TODO: Resolve warning!

    if (diskSpace < qint64(RDS_DISKLIMIT_ALTERNATING))
    {
        alternatingUpdate=true;
        RTI->log("Using alternating update mode due to low disk space.");
        RTI_NETLOG->postEvent(EventInfo::Type::Update,EventInfo::Detail::LowDiskSpace,EventInfo::Severity::Warning,"Using alternating update mode");
    }

    if (diskSpace < qint64(RDS_DISKLIMIT_WARNING))
    {
        RTI->log("");
        RTI->log("WARNING: Available disk space is very low (< 3 Gb).");
        RTI->log("WARNING: Exporting files from RAID might fail.");
        RTI->log("WARNING: Please free disk space.");
        RTI->log("");
        RTI->showOperationWindow();
        RTI_NETLOG->postEvent(EventInfo::Type::Update,EventInfo::Detail::LowDiskSpace,EventInfo::Severity::Error,"Critically low disk space");
    }

    RTI->log("");
    RTI->log("Starting update...");

    if (state!=STATE_IDLE)
    {
        RTI->log("Warning: Received update request in non-idle state");
    }

    RTI->setPostponementRequest(false);

    // Read the RAID directory, parse it, and decide which
    // scans have to be saved.
    RTI_RAID->createExportList();

    QUrlQuery data;
    for (rdsRaidEntry* entry: RTI_RAID->raidList){
        entry->addToQuery(data);
    }
    QNetworkReply::NetworkError error;
    int http_status = 0;
    bool success = RTI_NETWORK->netLogger->postData(data, "RaidRecords",error,http_status);
    if (!success) {
        if (http_status) {
            RTI->log(QString("Error: Scans could not be logged. (HTTP Error %1)").arg(http_status));
        } else {
            QMetaEnum metaEnum = QMetaEnum::fromType<QNetworkReply::NetworkError>();
            RTI->log(QString("Error: Scans could not be logged (%1)").arg(metaEnum.valueToKey(error)));
        }
    }
    // Check if the connection to the FTP server or network drive can be established
    if (RTI_NETWORK->openConnection())
    {
        explicitUpdate=false;
        connectionFailureCount=0;
        rdsActivityWindow* activityWindow=0;

        // If the operation window is not visible, show a notification dialog to the user
        if (showActivityWindow)
        {
            activityWindow=new rdsActivityWindow(0);
            activityWindow->show();
        }

        setState(STATE_RAIDTRANSFER);

        RTI->updateInfoUI();
        RTI->processEvents();



        // Process Windows events to react to the postpone button
        RTI->processEvents();

        bool exportSuccessful=true;

        // Decide if files should be exported and transfered at once
        // of if the files should be exported and transfered one by one.
        if (alternatingUpdate)
        {
            // Loop over all scans scheduled for the export
            while ((exportSuccessful) && (!RTI->isPostponementRequested()) && (RTI_RAID->exportsAvailable()))
            {
                // Save one file to the queue directory
                setState(STATE_RAIDTRANSFER);
                exportSuccessful=RTI_RAID->processExportListEntry();
                RTI->processEvents();

                // Transfer the file to the network
                setState(STATE_NETWORKTRANSFER_ALTERNATING);
                RTI_NETWORK->transferFiles();
                RTI->processEvents();
            }
            if (RTI->isPostponementRequested())
            {
                RTI->log("Received postponement request. Stopping update.");
            }
        }
        else
        {            
            // Export all scheduled scans to the queue directory
            exportSuccessful=RTI_RAID->processTotalExportList();
        }

        if (!exportSuccessful)
        {
            RTI->log("WARNING: Errors occured during export.");
            RTI->log("WARNING: Data transfer has been terminated.");
            RTI->setSevereErrors(true);
            RTI_NETLOG->postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Error, "Error during export");
        }

        // Close activity window if visible
        if (activityWindow!=0)
        {
            /*
            QElapsedTimer timer;
            timer.start();
            while (!timer.hasExpired(5000))
            {
                RTI->processEvents();
            }
            */

            RDS_FREE(activityWindow);
        }

        setState(STATE_NETWORKTRANSFER);
        RTI->updateInfoUI();
        RTI->processEvents();

        // Again, clean up the queue directoy
        RTI_NETWORK->transferFiles();

        RTI->log("Update finished.");

        // Copy the local logfile to the network drive for remote diagnosis
        RTI_NETWORK->copyLogFile();

        // Close connection to FTP server
        RTI_NETWORK->closeConnection();
    }
    else
    {
        RTI->log("Opening connection to target path failed.");
        RTI->log("Retrying in " + QString::number(RDS_UPDATETIME_RETRY) + " minutes.");
        setExplicitUpdate(RDS_UPDATETIME_RETRY);

        connectionFailureCount++;

        if (connectionFailureCount > RDS_CONNECTIONFAILURE_COUNT)
        {
            RTI->log("");
            RTI->log("NOTE: Opening the connection failed multiple times.");
            RTI->log("NOTE: Raw data will not be saved until the connection can be opened.");
            RTI->log("NOTE: Please check connection settings.");
            RTI->log("");
            connectionFailureCount=0;

            // Show the window so that the users get notified about the problem
            // NOTE: On request of the techs, showing the status window after
            //       failed updates was disabled.
            //RTI->showOperationWindow();
            RTI_NETLOG->postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::FatalError, "Opening the connection failed multiple times");
        } else {
            RTI_NETLOG->postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Error, "Opening the connection failed");
        }

    }

    lastUpdate=QDateTime::currentDateTime();
    setNextPeriodicUpdate();

    RTI->flushLog();
    setState(STATE_IDLE);
    RTI->updateInfoUI();
}


void rdsProcessControl::setStartTime()
{
    connectionFailureCount=0;

    startTime=QDateTime::currentDateTime();
    lastUpdate=startTime;
    lastCheckTime=startTime;

    state=STATE_IDLE;
    showActivityWindow=false;

    updateTime1Done=false;
    updateTime2Done=false;
    updateTime3Done=false;

    // Check if update times are in the past
    if (RTI_CONFIG->infoUpdateTime1<startTime.time())
    {
        updateTime1Done=true;
    }
    if (RTI_CONFIG->infoUpdateTime2<startTime.time())
    {
        updateTime2Done=true;
    }
    if (RTI_CONFIG->infoUpdateTime3<startTime.time())
    {
        updateTime3Done=true;
    }

    setNextPeriodicUpdate();

    // Set the update time if update mode has been set to startup
    if ((RTI_CONFIG->infoUpdateMode==RTI_CONFIG->UPDATEMODE_STARTUP) ||
        (RTI_CONFIG->infoUpdateMode==RTI_CONFIG->UPDATEMODE_STARTUP_FIXEDTIME) )
    {
        setExplicitUpdate(RDS_UPDATETIME_STARTUP);
    }
}


void rdsProcessControl::setNextPeriodicUpdate()
{
    nextPeriodicUpdate=lastUpdate;

    if (RTI_CONFIG->infoUpdatePeriodUnit==rdsConfiguration::PERIODICUNIT_MIN)
    {
        // Add update period in minutes
        nextPeriodicUpdate=nextPeriodicUpdate.addSecs(RTI_CONFIG->infoUpdatePeriod * 60);
    }
    else
    {
        // Add update period in hours
        nextPeriodicUpdate=nextPeriodicUpdate.addSecs(RTI_CONFIG->infoUpdatePeriod * 3600);
    }
}


