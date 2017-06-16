#include "rds_processcontrol.h"
#include "rds_global.h"
#include "rds_activitywindow.h"
#include "rds_operationwindow.h"
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

    bool updateNeeded=false;
    QDateTime currTime = QDateTime::currentDateTime();
    logServerOnlyUpdate=false;

    // Check the different update conditions

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
        if ((!updateTime1Done) && (currTime.time() > RTI_CONFIG->infoUpdateTime1Jittered))
        {
            updateTime1Done=true;
            updateNeeded=true;
            RTI->debug("Update condition: Fixed Time 1");
        }

        if ((RTI_CONFIG->infoUpdateUseTime2) && (!updateTime2Done) && (currTime.time() > RTI_CONFIG->infoUpdateTime2Jittered))
        {
            updateTime2Done=true;
            updateNeeded=true;
            RTI->debug("Update condition: Fixed Time 2");
        }

        if ((RTI_CONFIG->infoUpdateUseTime3) && (!updateTime3Done) && (currTime.time() > RTI_CONFIG->infoUpdateTime3Jittered))
        {
            updateTime3Done=true;
            updateNeeded=true;
            RTI->debug("Update condition: Fixed Time 3");
        }
    }

    if ((RTI_NETLOG.isConfigured()) && (RTI_CONFIG->logSendScanInfo) && (!updateNeeded))
    {
        if (currTime>nextLogServerOnlyUpdate)
        {
            updateNeeded=true;
            logServerOnlyUpdate=true;

            RTI->debug("Update condition: LogServer Only");
        }
    }

    lastCheckTime=currTime;

    return updateNeeded;
}


void rdsProcessControl::performUpdate()
{
    RTI->setIconWindowAnim(true);

    RTI->setSevereErrors(false);
    explicitUpdate=false;
    bool alternatingUpdate=false;
    qint64 diskSpace=RTI->getFreeDiskSpace();
    RTI->debug("Disk space on appdir = " + QString::number(diskSpace));

    // Use the alternating update mode when the disk space is below 5 Gb.
    // In the alternating mode, files are fetched from RAID and transferred
    // to the storage individually, which leads to longer dead time of the
    // scanner
    // TODO: Resolve warning!

    if (diskSpace < qint64(RDS_DISKLIMIT_ALTERNATING))
    {
        alternatingUpdate=true;
        RTI->log("Using alternating update mode due to low disk space.");
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::LowDiskSpace,EventInfo::Severity::Warning,"Using alternating update mode");
    }

    if (diskSpace < qint64(RDS_DISKLIMIT_WARNING))
    {
        RTI->log("");
        RTI->log("WARNING: Available disk space is very low (< 3 Gb).");
        RTI->log("WARNING: Exporting files from RAID might fail.");
        RTI->log("WARNING: Please free disk space.");
        RTI->log("");
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::LowDiskSpace,EventInfo::Severity::Error,"Critically low disk space");
        RTI->showOperationWindow();
    }

    RTI->log("");

    if (logServerOnlyUpdate)
    {
        RTI->log("Starting log update...");
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Start,EventInfo::Severity::Success,"Scan data");
    }
    else
    {
        RTI->log("Starting update...");
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Start,EventInfo::Severity::Success,"Raw data");
    }

    if (state!=STATE_IDLE)
    {
        RTI->log("Warning: Received update request in non-idle state");
    }

    RTI->setPostponementRequest(false);

    // The the scan list from the RAID
    if (!RTI_RAID->readRaidList())
    {
        RTI->log("ERROR: Unable to read RAID list.");
        RTI->log("ERROR: Retrying in " + QString::number(RDS_UPDATETIME_RETRY) + " minutes.");
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Error,"Unable to read RAID list");
        setExplicitUpdate(RDS_UPDATETIME_RETRY);

        // Indicate the error in the top icon
        if (!RTI->getWindowInstance()->isVisible())
        {
            RTI->getWindowInstance()->iconWindow.setError();
        }

        RTI->flushLog();
        setState(STATE_IDLE);
        RTI->updateInfoUI();

        RTI->setIconWindowAnim(false);
        RTI->log("");
        return;
    }

    // Transfer the raid scan table to the log server, if configured and desired
    if ((RTI_CONFIG->logSendScanInfo) && (RTI_NETLOG.isConfigured()))
    {
        sendScanInfoToLogServer();
    }

    // If a full update with raw-data transfer should be done
    if ((!logServerOnlyUpdate) && (!RTI_RAID->isScanActive()))
    {              
        // Check if the connection to the network drive can be established
        if (RTI_NETWORK->openConnection())
        {
            // Load the network protocols (if defined)
            if (!RTI_CONFIG->loadRemotelyDefinedProtocols())
            {
                // Indicate the error in the top icon
                if (!RTI->getWindowInstance()->isVisible())
                {
                    RTI->getWindowInstance()->iconWindow.setError();
                }
            }

            // Decide which scans should be saved
            RTI_RAID->createExportList();

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

                // Indicate the error in the top icon
                if (!RTI->getWindowInstance()->isVisible())
                {
                    RTI->getWindowInstance()->iconWindow.setError();
                }

                RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Error, "Error during export");
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

            RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Success,"Raw data sent");
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

                // Indicate the error in the top icon
                if (!RTI->getWindowInstance()->isVisible())
                {
                    RTI->getWindowInstance()->iconWindow.setError();
                }

                RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::FatalError, "Opening connection failed repeatedly");
            }
            else
            {
                RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Error, "Opening connection failed");
            }
        }

        lastUpdate=QDateTime::currentDateTime();
    }

    // If the current update run was for raw data transfer and a scan
    // is currently active, don't update and retry in some time
    if ((!logServerOnlyUpdate) && (RTI_RAID->isScanActive()))
    {
        RTI->log("Scan is active. Retrying in " + QString::number(RDS_UPDATETIME_RETRY) + " minutes.");
        setExplicitUpdate(RDS_UPDATETIME_RETRY);
    }

    lastLogServerOnlyUpdate=QDateTime::currentDateTime();
    setNextPeriodicUpdate();

    RTI->flushLog();
    setState(STATE_IDLE);
    RTI->updateInfoUI();

    RTI->setIconWindowAnim(false);
    RTI->log("");

    RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::End,EventInfo::Severity::Success,"");
}


void rdsProcessControl::sendScanInfoToLogServer()
{
    QUrlQuery data;

    if (RTI_RAID->raidList.isEmpty())
    {
        // RAID list is empty for whatever reason. Nothing needs to be done.
        return;
    }
    else
    {
        // If the fileID of the first (i.e. newest) scan is smaller than the LPFI,
        // then a reset of the RAID must have happened. Reset the LPFI and process
        // the whole RAID list then.
        if (RTI_RAID->raidList.at(0)->fileID<RTI_RAID->getLPFIScaninfo())
        {
            RTI_RAID->setLPFIScaninfo(-1);
        }
    }

    // Send the API key if it has been entered
    if (!RTI_CONFIG->logApiKey.isEmpty())
    {
        data.addQueryItem("api_key",RTI_CONFIG->logApiKey);
    }

    int entryCount=0;
    for (rdsRaidEntry* entry: RTI_RAID->raidList)
    {
        // Check if this entry has already been processed during the previous run.
        if (entry->fileID<=RTI_RAID->getLPFIScaninfo())
        {
            break;
        }
        entry->addToUrlQuery(data);
        entryCount++;
    }

    if (entryCount==0)
    {
        // No new entries to transfer, so just return
        return;
    }

    QNetworkReply::NetworkError error;
    int http_status=0;

    QString errorString="";
    bool success=RTI_NETWORK->netLogger.postData(data, NETLOG_ENDPT_RAIDLOG, error, http_status, errorString);

    if (!success)
    {
        if (http_status)
        {
            RTI->log(QString("ERROR: Transfer to log server failed (HTTP Error %1).").arg(http_status));           

            QString httpError="HTTP Error "+QString::number(http_status);
            RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error sending scan data: "+httpError);
        }
        else
        {
            RTI->log(QString("ERROR: Transfer to log server failed (%1).").arg(errorString));
            RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error sending scan data: "+errorString);
        }

        // Indicate the error in the top icon
        if (!RTI->getWindowInstance()->isVisible())
        {
            RTI->getWindowInstance()->iconWindow.setError();
        }
    }
    else
    {
        // Store the fileID of the newest RAID entry processed, so that
        // during the next run only new entries are processed
        RTI_RAID->setLPFIScaninfo(RTI_RAID->raidList.at(0)->fileID);
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Information,EventInfo::Severity::Success,"Scan data sent");
    }

    RTI_RAID->saveLPFI();
}


void rdsProcessControl::setStartTime()
{
    connectionFailureCount=0;

    startTime=QDateTime::currentDateTime();
    lastUpdate=startTime;
    lastCheckTime=startTime;
    lastLogServerOnlyUpdate=startTime;

    state=STATE_IDLE;
    showActivityWindow=false;

    updateTime1Done=false;
    updateTime2Done=false;
    updateTime3Done=false;

    // Check if update times are in the past
    if (RTI_CONFIG->infoUpdateTime1Jittered<startTime.time())
    {
        updateTime1Done=true;
    }
    if (RTI_CONFIG->infoUpdateTime2Jittered<startTime.time())
    {
        updateTime2Done=true;
    }
    if (RTI_CONFIG->infoUpdateTime3Jittered<startTime.time())
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

    logServerOnlyUpdate=false;
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

    // Calculate the time for the next logserver-only intermediate update
    if (RTI_NETLOG.isConfigured() && (RTI_CONFIG->logSendScanInfo))
    {
        nextLogServerOnlyUpdate=lastLogServerOnlyUpdate;
        nextLogServerOnlyUpdate=nextLogServerOnlyUpdate.addSecs(RTI_CONFIG->logUpdateFrequency * 3600);
    }
}

