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
        RTI->log("WARNING: Received update request in non-idle state");
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
    // TODO: Resolve compiler warning

    if (!RTI->isSimulation() && diskSpace < qint64(RDS_DISKLIMIT_ALTERNATING))
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
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Start,EventInfo::Severity::Success,"ScanInfo only");
    }
    else
    {
        RTI->log("Starting update...");
        RTI_NETLOG.postEvent(EventInfo::Type::Update,EventInfo::Detail::Start,EventInfo::Severity::Success,"");
    }

    if (state!=STATE_IDLE)
    {
        RTI->log("WARNING: Received update request in non-idle state");
    }

    RTI->setPostponementRequest(false);

    setState(STATE_RAIDPARSING);

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

    setState(STATE_SCANTRANSFER);

    // Transfer the raid scan table to the log server, if configured and desired
    if ((RTI_CONFIG->logSendScanInfo) && (!RTI_CONFIG->logServerPath.isEmpty()))
    {
        RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Start,EventInfo::Severity::Success,"");

        if (!RTI_NETLOG.isConfigurationError())
        {
            resendScanInfoFromDisk();
        }

        sendScanInfoToLogServer();

        RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::End,EventInfo::Severity::Success,"");
    }

    // If a full update with raw-data transfer should be done
    if ((!logServerOnlyUpdate) && (!RTI_RAID->isScanActive()))
    {
        RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Start,EventInfo::Severity::Success,"");

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

            // Process Windows events to react to the postpone button
            RTI->processEvents();

            bool exportSuccessful=true;

            // Decide if files should be exported and transfered at once
            // of if the files should be exported and transfered one by one.
            if (alternatingUpdate)
            {
                // Loop over all scans scheduled for the export
                while ((exportSuccessful) && (!RTI->isPostponementRequested()) && (RTI_RAID->exportsAvailable() > 0))
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
                int exportCount = 0;

                // If exporting the list fails for lack of space, export what we can and try again
                // Worst case we export one file at a time.

                RTI->log("===== Begin Export & Transfer =====");
                int totalToExport = RTI_RAID->exportsAvailable();
                int totalExported = 0;
                for (int i=0; i < totalToExport + 1; i++) {
                    bool diskFull = false;
                    RTI->log("==== Beginning RAID file export. ====");
                    exportSuccessful = RTI_RAID->processTotalExportList(diskFull, exportCount);
                    totalExported += exportCount;
                    if ( exportSuccessful ) {
                        RTI->log("===== RAID export SUCCESS =====");
                        RTI->log("Exported "+ QString::number(totalExported) + " of "+ QString::number(totalToExport) + " files.");
                        break; // we exported everything, we're done (transfer happens later)
                    } else if ( diskFull ) { // It failed due to disk space
                        if ( exportCount == 0 ) { // If it couldn't even export one file, we are out of options.
                            RTI->log("ERROR: Critical disk space error.");
                            break;
                        }
                        RTI->log("WARNING: RAID file export ran out of disk space.");
                        RTI->log("==== Transfering " + QString::number(exportCount) + " files. ====");
                        setState(STATE_NETWORKTRANSFER_ALTERNATING); // not sure about this
                        RTI_NETWORK->transferFiles(); // transfer the files we were able to export
                        RTI->processEvents();
                        RTI->log("===== Finished export phase "+ QString::number(i+1) + " =====");
                    } else { // something else went wrong
                        RTI->log("===== RAID export FAILURE =====");
                        break;
                    }
                }
            }

            if (!exportSuccessful)
            {
                RTI->log("WARNING: Errors occured during export.");
                RTI->log("WARNING: Data transfer has been terminated.");
                RTI->setSevereErrors(true);
                RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Information,EventInfo::Severity::Error, "Error during export");

                // Indicate the error in the top icon
                if (!RTI->getWindowInstance()->isVisible())
                {
                    RTI->getWindowInstance()->iconWindow.setError();
                }
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
            bool was_error = RTI->isSevereErrors();
            RTI_NETWORK->transferFiles();

            if (!was_error && RTI->isSevereErrors()) {
                QString configFileData;
                QString logFileData;
                try {
                    logFileData = RTI_NETWORK->getLogFileData(50);
                    configFileData = RTI_NETWORK->getConfigFileData();
                } catch (...) {
                    logFileData = "";
                    configFileData = "";
                }
                RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Diagnostics,EventInfo::Severity::Error, "File transfer generated error(s).", logFileData);
                RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Diagnostics,EventInfo::Severity::Error, "Config file", configFileData);

            }

            RTI->log("Update finished.");

            // Copy the local logfile to the network drive for remote diagnosis
            if (!RTI_NETWORK->copyLogFile()) {
                QString logFileData;
                try {
                    logFileData = RTI_NETWORK->getLogFileData(100);
                } catch (...) {
                    logFileData = "";
                }
                RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Diagnostics,EventInfo::Severity::Error, "Log file transfer failed.", logFileData);
            }

            // Close connection to FTP server
            RTI_NETWORK->closeConnection();

            RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Information,EventInfo::Severity::Success,"Data transfer ended");
        }
        else
        {
            RTI->log("Opening connection to target path failed.");
            RTI->log("Retrying in " + QString::number(RDS_UPDATETIME_RETRY) + " minutes.");
            setExplicitUpdate(RDS_UPDATETIME_RETRY);

            connectionFailureCount++;
            QString logFileData;
            try {
                logFileData = RTI_NETWORK->getLogFileData(10);
            } catch (...) {
                logFileData = "";
            }
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

                RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Information,EventInfo::Severity::FatalError, "Opening connection failed repeatedly",logFileData);
                RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Diagnostics,EventInfo::Severity::FatalError, "RDS configuration",RTI_NETWORK->getConfigFileData());
            }
            else
            {
                RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::Information,EventInfo::Severity::Error, "Opening connection failed",logFileData);

                // Rerun the startup commands if reconnecting failed for three times. Maybe this will
                // resolve the connection problem.
                if ((RTI_CONFIG->netDriveStartupCmdsAfterFail) &&
                    (connectionFailureCount > RDS_STARTUPCMDAFTERFAIL_COUNT))
                {
                    RTI->getWindowInstance()->runStartCmds();
                }
            }

        }

        lastUpdate=QDateTime::currentDateTime();

        RTI_NETLOG.postEvent(EventInfo::Type::RawDataStorage,EventInfo::Detail::End,EventInfo::Severity::Success,"");
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
        if (RTI_RAID->raidList.at(0)->fileID < RTI_RAID->getLPFIScaninfo())
        {
            RTI_RAID->setLPFIScaninfo(-1);
        }
    }

    // Send the API key if it has been entered
    if (!RTI_CONFIG->logApiKey.isEmpty())
    {
        data.addQueryItem("api_key",RTI_CONFIG->logApiKey);
    }

    int     entryCount=0;
    bool    isDummyScan=false;
    QString originalProtocolName="";
    bool    actualScanFound=false;

    for (rdsRaidEntry* entry: RTI_RAID->raidList)
    {
        // Check if this entry has already been processed during the previous run.
        if (entry->fileID <= RTI_RAID->getLPFIScaninfo())
        {
            break;
        }

        // Send the entry, except if it has the error attribute. If it's a tagging scan, then send
        // it in any case, even with error tag
        if ((entry->attribute!=RDS_SCANATTRIBUTE_ERROR) || (entry->protName.startsWith("$")))
        {
            entry->addToUrlQuery(data);
            entryCount++;
        }
    }

    if (entryCount==0)
    {
        // No new entries to transfer, so just return
        return;
    }

    bool storeScanInfo=false;

    // Only send the scan info if the connection to the server has been DNS validated
    if (!RTI_NETWORK->netLogger.isConfigurationError())
    {        
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
                RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Push,EventInfo::Severity::Error,"Error: "+httpError);
            }
            else
            {
                RTI->log(QString("ERROR: Transfer to log server failed (%1).").arg(errorString));
                RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Push,EventInfo::Severity::Error,"Error: "+errorString);
            }

            // Indicate the error in the top icon
            if (!RTI->getWindowInstance()->isVisible())
            {
                RTI->getWindowInstance()->iconWindow.setError();
            }

            storeScanInfo=true;
        }
        else
        {
            RTI->log("Scaninfo sent.");
            RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Push,EventInfo::Severity::Success,"");
        }
    }
    else
    {
        // If the server connection could not be validated, store the scan info
        // on the local drive. It is assumed that the logserver or DNS server is
        // temporarily down.
        storeScanInfo=true;
    }

    // If the transfer was not successful or the connection could not be validated,
    // store the scan info locally in a temporary file and resend later
    if (storeScanInfo)
    {
        RTI->log("WARNING: Storing scan info locally.");
        storeScanInfoOnDisk(data);
    }

    // Store the fileID of the newest RAID entry processed, so that
    // during the next run only new entries are processed
    RTI_RAID->setLPFIScaninfo(RTI_RAID->raidList.at(0)->fileID);
    RTI_RAID->saveLPFI();
}


void rdsProcessControl::storeScanInfoOnDisk(QUrlQuery& data)
{
    // Generate a unique file name for temporarily storing the scan info
    QString timeStamp=QDateTime::currentDateTime().toString("MMddyy_HHmmss_zzz");
    QString fileName=RTI->getAppPath()+"/"+timeStamp+RDS_SCANINFO_EXT;

    // Check if filename exists. If so, add random number until non-existent filename is reached.
    int tries=0;
    while (QFile::exists(fileName))
    {
        fileName=RTI->getAppPath()+"/"+timeStamp+"_"+QString::number(qrand() % 1000)+RDS_SCANINFO_EXT;

        // Abort after some tries (very very unlikely, so something else must be wrong)
        tries++;
        if (tries>20)
        {
            RTI->log(QString("ERROR: Unable to generate unique filename for storing scan info (%1).").arg(fileName));
            return;
        }
    }

    QFile bufferFile(fileName);

    if (!bufferFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        RTI->log(QString("ERROR: Unable to create file for storing scan info (%1).").arg(fileName));
        return;
    }

    QTextStream stream(&bufferFile);

    stream << RDS_SCANINFO_HEADER << endl;
    stream << data.queryItems().count() << endl;

    // Write data information
    for (int i=0; i<data.queryItems().count(); i++)
    {
        stream << data.queryItems().at(i).first  << endl;
        stream << data.queryItems().at(i).second << endl;
    }

    bufferFile.flush();
    bufferFile.close();
}


void rdsProcessControl::resendScanInfoFromDisk()
{
    // Make sure that the configuration to the log server has been validated
    if (RTI_NETLOG.isConfigurationError())
    {
        return;
    }

    // Search for all files with extension .ylb in the application folder. Sort the list
    // by date so that the oldest scans are getting pushed first
    QDir folder(RTI->getAppPath());
    folder.refresh();    
    QStringList bufferFiles=folder.entryList(QStringList(QString("*")+RDS_SCANINFO_EXT),QDir::NoFilter,QDir::Time|QDir::Reversed);

    if (bufferFiles.isEmpty())
    {
        // No files found, nothing to do.
        return;
    }

    RTI->log("WARNING: Scan info from previous update found.");
    RTI->log("Resending stored data to server.");
    RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Information,EventInfo::Severity::Warning,"Resend data found");

    for (int i=0; i<bufferFiles.count(); i++)
    {        
        QString fileName=RTI->getAppPath()+"/"+bufferFiles.at(i);
        QFile   bufferFile(fileName);

        // Open file, check is successful
        if (!bufferFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            RTI->log(QString("ERROR: Unable to open scan-info file (%1).").arg(fileName));
            continue;
        }

        // Read file contents
        QString buffer="";
        QTextStream stream(&bufferFile);

        // Read the header and check if it is correct
        if (!stream.readLineInto(&buffer))
        {
            RTI->log(QString("ERROR: Scan-info file is empty (%1).").arg(fileName));
            bufferFile.close();
            continue;
        }

        if (buffer != QString(RDS_SCANINFO_HEADER))
        {
            RTI->log(QString("ERROR: Scan-info file is invalid (%1).").arg(fileName));
            bufferFile.close();
            continue;
        }

        // Read the entry count and check if the second line contains a number
        if (!stream.readLineInto(&buffer))
        {
            RTI->log(QString("ERROR: Scan-info file is invalid (%1).").arg(fileName));
            bufferFile.close();
            continue;
        }

        int expectedEntries=0;
        expectedEntries=buffer.toInt();

        if (expectedEntries <= 0)
        {
            RTI->log(QString("ERROR: Scan-info file is invalid (%1).").arg(fileName));
            bufferFile.close();
            continue;
        }

        QUrlQuery query;
        QString   key  ="";
        QString   value="";

        int foundEntries=0;
        int lineCounter =0;

        // Read the whole file
        while (stream.readLineInto(&buffer))
        {
            if ((lineCounter % 2)==0)
            {
                key=buffer;
            }
            else
            {
                value=buffer;

                // Update the api key (in case it has been changed)
                if (key=="api_key")
                {
                    value=RTI_CONFIG->logApiKey;
                }

                query.addQueryItem(key,value);
                foundEntries++;
            }

            lineCounter++;
        }

        // Check if number of entries is consistent
        if (expectedEntries!=foundEntries)
        {
            RTI->log(QString("ERROR: Scan-info file is inconsistent (%1).").arg(fileName));
            RTI->log(QString("ERROR: Entries expected=%1 Entries found=%2.").arg(expectedEntries).arg(foundEntries));
            bufferFile.close();
            continue;
        }

        // Close file
        bufferFile.close();

        // Send data to server
        QNetworkReply::NetworkError error;
        int http_status=0;
        QString errorString="";

        bool success=RTI_NETWORK->netLogger.postData(query, NETLOG_ENDPT_RAIDLOG, error, http_status, errorString);

        if (!success)
        {
            if (http_status)
            {
                RTI->log(QString("ERROR: Transfer to log server failed (HTTP Error %1).").arg(http_status));
                QString httpError="HTTP Error "+QString::number(http_status);
                RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Push,EventInfo::Severity::Error,"Resend error: "+httpError);
            }
            else
            {
                RTI->log(QString("ERROR: Transfer to log server failed (%1).").arg(errorString));
                RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Push,EventInfo::Severity::Error,"Resend error: "+errorString);
            }

            // Server transfer seems still to have problems. So stop here and continue during the next update
            break;
        }
        else
        {
            // Transfer sucessful, delete file
            bufferFile.remove();
            RTI_NETLOG.postEvent(EventInfo::Type::ScanInfo,EventInfo::Detail::Push,EventInfo::Severity::Success,"Resend successful");
        }
    }
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

        // Calculate the update time
        int updateTime=RTI_CONFIG->logUpdateFrequency * 3600;

        // If update time is given in minutes
        if (RTI_CONFIG->logUpdateFrequencyUnit==1)
        {
            updateTime=RTI_CONFIG->logUpdateFrequency * 60;
        }

        nextLogServerOnlyUpdate=nextLogServerOnlyUpdate.addSecs(updateTime);
    }
}

