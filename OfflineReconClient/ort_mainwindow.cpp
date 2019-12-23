
#include <QtWidgets>
#include "ort_mainwindow.h"
#include "ui_ort_mainwindow.h"

#include "ort_global.h"
#include "ort_confirmationdialog.h"
#include "ort_waitdialog.h"
#include "ort_copydialog.h"
#include "ort_recontask.h"
#include "ort_bootdialog.h"
#include "ort_configurationdialog.h"


ortMainWindow::ortMainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortMainWindow)
{
    selectedPatient="";
    selectedScantime="";
    selectedProtocol="";
    selectedMode=0;
    selectedFID=-1;

    ui->setupUi(this);
    setWindowIcon(ORT_ICON);
    setWindowTitle("Yarra - Offline Reconstruction Task");

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    log.start();
    RTI->setLogInstance(&log);
    // Only to make sure no null-pointer calls occur by accident
    RTI->setConfigInstance(&dummyconfig);
    RTI->setRaidInstance(&raid);

    cloud.setConfiguration(&cloudConfig);
    cloudConfig.loadConfiguration();

    // Tell the raid class to not use the LPFI mechanism (which was designed for RDS).
    raid.setIgnoreLPFI();
    isRaidListAvaible=false;

    // Load the configuration for getting the information where the ORT directory is located.
    config.loadConfiguration();

    if (!config.isConfigurationValid())
    {
        // Configuration is incomplete, so shut down
        QMessageBox msgBox;
        msgBox.setWindowTitle("Configuration Invalid");
        msgBox.setText("The Yarra ORT client has not been configured yet.\n\nDo you want to open the configuration?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setWindowIcon(ORT_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        if (msgBox.exec()==QMessageBox::Yes)
        {
            // Call configuration dialog
            ortConfigurationDialog::executeDialog();
        }

        RTI->log("Invalid configuration. Terminating.");

        // Shutdown the client
        QTimer::singleShot(0, qApp, SLOT(quit()));        
        return;
    }

    // Forward system name (necessary to define filename of the exported scans)
    raid.setORTSystemName(config.ortSystemName);

    RTI->log("System: "+config.ortSystemName);
    RTI->log("Serial: "+config.infoSerialNumber);
    RTI->log("Type:   "+config.infoScannerType);

    // Show a splash screen while the network connection is established, so that the user
    // knows that something is going on.
    ortBootDialog bootDialog;
    bootDialog.show();
    RTI->processEvents();

    network.setConfigInstance(&config);
    if (!network.prepare())
    {
        QTimer::singleShot(0, qApp, SLOT(quit()));
        return;
    }

    // Send the version number and name along with the boot notification
    QString dataString="<data>";
    dataString+="<version>"       +QString(ORT_VERSION)                   +"</version>";
    dataString+="<name>"          +QString(config.ortSystemName)          +"</name>";
    dataString+="<system_model>"  +QString(config.infoScannerType)        +"</system_model>";
    dataString+="<system_version>"+QString(RTI->getSyngoMRVersionString())+"</system_version>";
    dataString+="<system_vendor>Siemens</system_vendor>";
    dataString+="<time>"          +QDateTime::currentDateTime().toString()+"</time>";
    dataString+="</data>";
    network.netLogger.postEvent(EventInfo::Type::Boot,EventInfo::Detail::Information,EventInfo::Severity::Success,"Ver "+QString(ORT_VERSION),dataString);

    // Connect to the on-premise server if a server path has been defined. If not and
    // cloud support is disabled, also call it so that an error message appears. Do not
    // call it if no server path has been defined but cloud support is enabled (because
    // the user might only use cloud recons).
    if ((!config.ortServerPath.isEmpty()) || (!config.ortCloudSupportEnabled))
    {
        // Establish the connection to the yarra server
        if (!network.openConnection())
        {
            bool connectError=true;

            // Check if a fallback server has been defined
            if (network.fallbackConnectCmd.length()>0)
            {
                bootDialog.setFallbacktext();

                // Connect to the fallback server. If successful, then discard
                // the connection error and continue
                if (network.openConnection(true))
                {
                    connectError=false;
                }
            }

            if (connectError)
            {
                network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"No connection to server");
                QTimer::singleShot(0, qApp, SLOT(quit()));
                return;
            }
        }

        // OK, now connect to the ORT directory, read the configuration
        // from there, and read the RAID list.

        modeList.network=&network;

        if (!modeList.readModeList())
        {
            network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Unable to read mode list");
            QTimer::singleShot(0, qApp, SLOT(quit()));
            return;
        }
    }

    // Now read cloud modes if cloud support has been enabled
    bool cloudModeLoaded=false;

    if (config.ortCloudSupportEnabled)
    {
        if (!cloud.createCloudFolders())
        {
            RTI->log("ERROR: Preparing cloud folder failed.");

            showCloudProblem("Unable to prepare folder for cloud reconstruction.<br>Please check you local client installation.");
            return;
        }

        bootDialog.setText("Connecting to YarraCloud. Please wait...");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        qApp->processEvents();

        if (!cloud.validateUser())
        {
            QApplication::restoreOverrideCursor();
            showCloudProblem("Unable to validate your YarraCloud account.<br>You will not be able to perform cloud reconstructions.<br><br>Reason: " + cloud.errorReason);
            return;
        }

        // Connect to the cloud service and read the list of cloud modes
        int cloudModes=cloud.readModeList(&modeList);

        if (cloudModes<0)
        {
            QApplication::restoreOverrideCursor();
            showCloudProblem("Unable to read mode list from cloud.<br><br>Reason: " + cloud.errorReason);
            return;
        }

        if (cloudModes>0)
        {
            cloudModeLoaded=true;
        }

        QApplication::restoreOverrideCursor();
    }

    // Show the readable names of the protocols in the UI
    for (int i=0; i<modeList.modes.count(); i++)
    {
        ui->modeComboBox->addItem(modeList.modes.at(i)->readableName);

        // If at least one cloud mode has been loaded, add icons to
        // indicate where the modes are reconstructed
        if (cloudModeLoaded)
        {
            switch (modeList.modes.at(i)->computeMode)
            {
            default:
            case ortModeEntry::OnPremise:
                ui->modeComboBox->setItemIcon(i,QIcon(":/images/premise.png"));
                break;

            case ortModeEntry::Cloud:
                ui->modeComboBox->setItemIcon(i,QIcon(":/images/cloud.png"));
                break;

            case ortModeEntry::Elastic:
                ui->modeComboBox->setItemIcon(i,QIcon(":/images/elastic.png"));
                break;
            }
        }
    }

    bootDialog.close();

    isManualAssignment=false;
    ui->modeComboBox->setEnabled(false);

    ui->scansWidget->setColumnHidden(5, true);
    ui->scansWidget->setColumnHidden(6, true);

    // Set reasonable sizes for columns
    ui->scansWidget->horizontalHeader()->resizeSection(0,50);
    ui->scansWidget->horizontalHeader()->resizeSection(1,210);
    ui->scansWidget->horizontalHeader()->resizeSection(2,210);
    ui->scansWidget->horizontalHeader()->resizeSection(3,120);

    QFont font = ui->scansWidget->font();
    ui->scansWidget->horizontalHeader()->setFont( font );

    scansToShow=ORT_SCANSHOW_DEF;
    updateScanList();
}


ortMainWindow::~ortMainWindow()
{
    network.closeConnection();

    if (config.ortStartRDSOnShutdown)
    {
        QProcess::startDetached(qApp->applicationDirPath() + "/RDS.exe -silent");
    }

    if (config.ortCloudSupportEnabled)
    {
        cloud.launchCloudAgent();
    }

    RTI->log("Shutdown.");
    RTI->setLogInstance(0);
    log.finish();

    delete ui;
}


void ortMainWindow::addScanItem(int mid, QString patientName, QString protocolName, QDateTime scanTime, qint64 scanSize, int fid, int mode)
{
    ui->scansWidget->insertRow(ui->scansWidget->rowCount());
    int myRow=ui->scansWidget->rowCount()-1;

    QTableWidgetItem *myItem=0;

    // Col MID
    myItem=new QTableWidgetItem;
    myItem->setText(QString::number(mid));
    myItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->scansWidget->setItem(myRow,0,myItem);

    // Col Patient Name
    myItem=new QTableWidgetItem;
    myItem->setText(patientName);
    myItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->scansWidget->setItem(myRow,1,myItem);

    // Col Protocol Name
    myItem=new QTableWidgetItem;
    myItem->setText(protocolName);
    myItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->scansWidget->setItem(myRow,2,myItem);

    // Col Scan Time
    myItem=new QTableWidgetItem;
    QString timeString=scanTime.toString("dd/MM/yy")+"  "+scanTime.toString("HH:mm:ss");
    myItem->setText(timeString);
    myItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->scansWidget->setItem(myRow,3,myItem);

    // Col Size
    myItem=new QTableWidgetItem;
    double mbSize=scanSize/1048576.;
    myItem->setText(QString::number(mbSize,'f',1));
    myItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->scansWidget->setItem(myRow,4,myItem);

    // Col FID
    myItem=new QTableWidgetItem;
    myItem->setText(QString::number(fid));
    ui->scansWidget->setItem(myRow,5,myItem);

    // Col Mode
    myItem=new QTableWidgetItem;
    myItem->setText(QString::number(mode));
    ui->scansWidget->setItem(myRow,6,myItem);
}


void ortMainWindow::updateScanList()
{    
    ui->refreshButton->setEnabled(false);
    ui->loadOlderButton->setEnabled(false);
    ui->manualAssignButton->setEnabled(false);

    // Clear old items
    int rowCount=ui->scansWidget->rowCount();
    for (int i=0; i<rowCount; i++)
    {
        ui->scansWidget->removeRow(rowCount-1-i);
    }

    // Read the scan list from the RaidTool
    if (!isRaidListAvaible)
    {
        refreshRaidList();
    }

    // Estimate how many items should be dispayed
    int itemsToShow=scansToShow;

    // In the manual assignment mode, show 10 times more scans
    // because of all the adjustment scans
    if (isManualAssignment)
    {
        itemsToShow=scansToShow*ORT_SCANSHOW_MANMULT;

        // Never show more scans than the cap on items to-be-read
        // from RAID (should not happen anyway)
        if (itemsToShow>ORT_RAID_MAXPARSECOUNT)
        {
            itemsToShow=ORT_RAID_MAXPARSECOUNT;
        }
    }

    bool finished=false;
    int i=0;
    int itemCount=0;

    while (!finished)
    {
        if (i<raid.raidList.count())
        {
            rdsRaidEntry* entry=raid.raidList.at(i);
            int assignedMode=modeList.getModeForProtocol(entry->protName);

            // Exclude measurements that are too small, depending on the setting of the
            // reconstruction mode. This helps to exclude shimscans, which have the same
            // name of the RAID.
            if (assignedMode>-1)
            {
                qint64 minSize=modeList.modes.at(assignedMode)->minimumSizeMB*1048576.;
                if (entry->size<minSize)
                {
                    assignedMode=-1;
                }
            }

            if ((assignedMode>-1) || (isManualAssignment))
            {
                addScanItem(entry->measID, entry->patName, entry->protName, entry->creationTime, entry->size, entry->fileID, assignedMode);
                itemCount++;
            }

            if (itemCount>=itemsToShow)
            {
                finished=true;
            }
        }
        else
        {
            finished=true;
        }
        i++;
    }

    // Select the first row if there are any scans
    if (ui->scansWidget->rowCount()>0)
    {
        ui->scansWidget->setRangeSelected(QTableWidgetSelectionRange(0,0,0,4),true);
        ui->sendButton->setEnabled(true);
    }
    else
    {
        ui->sendButton->setEnabled(false);
    }

    on_scansWidget_itemSelectionChanged();

    ui->refreshButton->setEnabled(true);
    ui->loadOlderButton->setEnabled(true);
    ui->manualAssignButton->setEnabled(true);
}


void ortMainWindow::on_cancelButton_clicked()
{
    close();
}


void ortMainWindow::on_sendButton_clicked()
{
    selectedFID=-1;
    selectedPatient="";
    selectedScantime="";
    selectedProtocol="";
    selectedMode=ui->modeComboBox->currentIndex();

    // Identify clicked item
    if (ui->scansWidget->selectedRanges().count()>0)
    {
        int selectedRow=ui->scansWidget->selectedRanges().at(0).topRow();
        if (selectedRow>=0)
        {
            selectedFID     =ui->scansWidget->item(selectedRow,5)->text().toInt();
            selectedPatient =ui->scansWidget->item(selectedRow,1)->text();
            selectedScantime=ui->scansWidget->item(selectedRow,3)->text();
            selectedProtocol=ui->scansWidget->item(selectedRow,2)->text();
        }
    }
    else
    {
        // Nothing selected. In this case, the button should be disabled anyway.
        return;
    }

    if (selectedFID==-1)
    {
        RTI->log("ERROR: Invalid FID after pressing Send button");
        network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Invalid FID after pressing Send button");
        showTransferError("Invalid FID has been selected.");
        return;
    }

    // Configure configuration dialog
    ortConfirmationDialog confirmationDialog;
    confirmationDialog.setConfigInstance(&config);
    confirmationDialog.setPatientInformation(selectedPatient+",  "+selectedScantime);

    // Retrive the settings of the selected mode
    if ((selectedMode<0) or (selectedMode>=modeList.modes.count()))
    {
        RTI->log("ERROR: Invalid mode has been selected.");
        network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Invalid mode has been selected");
        showTransferError("Invalid reconstruction mode has been selected.");
        return;
    }

    if (modeList.modes.at(selectedMode)->computeMode!=ortModeEntry::OnPremise)
    {
        confirmationDialog.setCloudRecon();
    }

    if (modeList.modes.at(selectedMode)->requiresACC)
    {
        confirmationDialog.setACCRequired();
    }

    bool paramRequested=modeList.modes.at(selectedMode)->paramLabel!="";
    if (paramRequested)
    {
        ortModeEntry* mode=modeList.modes.at(selectedMode);
        confirmationDialog.setParamRequired(mode->paramLabel, mode->paramDescription, mode->paramDefault, mode->paramMin, mode->paramMax);
    }
    confirmationDialog.updateDialogHeight();
    confirmationDialog.exec();

    if (!confirmationDialog.isConfirmed())
    {
        // User decided to not send
        return;
    }

    this->hide();

    // Go ahead with the submission
    ortReconTask reconTask;
    reconTask.setInstances(&raid, &network, &config);
    reconTask.setCloudPaths(cloud.getCloudPath(YCT_CLOUDFOLDER_OUT), cloud.getCloudPath(YCT_CLOUDFOLDER_PHI));
    reconTask.reconMode       =modeList.modes.at(selectedMode)->idName;
    reconTask.reconName       =modeList.modes.at(selectedMode)->readableName;
    reconTask.systemName      =config.ortSystemName;
    reconTask.patientName     =selectedPatient;
    reconTask.scanProtocol    =selectedProtocol;
    reconTask.raidCreationTime=selectedScantime;

    // Initialize with value from the mode definition
    reconTask.emailNotifier=modeList.modes.at(selectedMode)->mailConfirmation;

    // Attach the entry from the dialog. Add separator character if needed
    QString mailRecipient=confirmationDialog.getEnteredMail();
    if ((reconTask.emailNotifier!="") && (mailRecipient!=""))
    {
        reconTask.emailNotifier+=",";
    }
    reconTask.emailNotifier  +=mailRecipient;

    if (modeList.modes.at(selectedMode)->requiresACC)
    {
        reconTask.accNumber=confirmationDialog.getEnteredACC();
    }

    if (paramRequested)
    {
        reconTask.paramValue=confirmationDialog.getEnteredParam();
    }

    // If the selected recon is a cloud mode, delegate the submission to a separate method and
    // return. Otherwise, continue with the usual local-server submission
    if (modeList.modes.at(selectedMode)->computeMode!=ortModeEntry::OnPremise)
    {
        if (processCloudRecon(reconTask))
        {
            this->close();
        }
        else
        {
            this->show();
        }
        return;
    }

    ortWaitDialog waitDialog;
    waitDialog.show();
    RTI->processEvents();

    // Tell the network module the name and type of the current server
    network.currentServer       =modeList.serverName;
    QString requiredServerType  =modeList.modes.at(selectedMode)->requiredServerType;
    reconTask.requiredServerType=requiredServerType;

    // Try to connect to a matching server (or switch server for load balancing)
    if (!network.reconnectToMatchingServer(requiredServerType))
    {
        // Error handling
        waitDialog.close();
        network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Unable to connect to required server");
        showTransferError(network.errorReason);
        this->show();
        return;
    }
    reconTask.selectedServer=network.selectedServer;

    RTI->log("Reconstruction request submitted (ORT client " + QString(ORT_VERSION) + ")");
    RTI->log("Selected server: "+network.selectedServer);

    if (ui->priorityButton->isChecked())
    {
        reconTask.highPriority=true;
    }

    if (!reconTask.exportDataFiles(selectedFID, modeList.modes.at(selectedMode)))
    {
        network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error exporting file: "+reconTask.getErrorMessageUI());
        showTransferError(reconTask.getErrorMessageUI());
        this->show();
        return;
    }

    waitDialog.close();

    ortCopyDialog copyDialog;
    copyDialog.show();

    if (!reconTask.transferDataFiles())
    {
        copyDialog.close();

        if (reconTask.fileAlreadyExists)
        {
            // Inform user that the file is already existing, i.e. the
            // task had already been submitted
            QString infoText="The scan selected for offline reconstruction is already present at the server. Likely, the scan has been submitted before for reconstruction.<br><br>";
            infoText+="If the a different problem exists with the reconstruction, please ask the administrators to manually remove the file from the server's input folder.<br><br>";
            infoText+="Filename: " + reconTask.scanFile;

            QMessageBox msgBox;
            msgBox.setWindowTitle("Scan already on server");
            msgBox.setText(infoText);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setWindowIcon(ORT_ICON);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();

            this->close();
        }
        else
        {
            network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error transfering file: "+reconTask.getErrorMessageUI());
            showTransferError(reconTask.getErrorMessageUI());
            this->show();
        }
        return;
    }

    if (!reconTask.generateTaskFile())
    {
        copyDialog.close();
        network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error generating task: "+reconTask.getErrorMessageUI());
        showTransferError(reconTask.getErrorMessageUI());
        this->show();
        return;
    }

    copyDialog.close();

    // Clean local queue directory (in any case)
    network.cleanLocalQueueDir();

    // If the reconstruction tasked has been submitted successfully, then
    // shutdown the ORT client.
    if (reconTask.isSubmissionSuccessful())
    {
        //RTI->log(getTaskInfo(reconTask));
        QString taskInfo=getTaskInfo(reconTask);
        network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::End,EventInfo::Severity::Success,taskInfo);
        this->close();
    }
    else
    {
        network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error with task submission: "+reconTask.getErrorMessageUI());
        showTransferError(reconTask.getErrorMessageUI());
        this->show();
    }
}


QString ortMainWindow::getTaskInfo(ortReconTask& task)
{
    return " ACC=" + task.accNumber + " MOD=" + task.reconMode + " SRV=" + task.selectedServer + " PRO='" + task.scanProtocol + "' FIL=" + task.scanFile;
}


void ortMainWindow::showTransferError(QString msg)
{
    QString errorText="Submission of the offline-reconstruction task was <b>not successful</b> due to the following reason:<br><br>";
    errorText+="<b>"+msg+"</b><br><br>";
    errorText+="More detailed information can be found in the log file. Please contact your administrator team if you can't resolve this problem.";

    QMessageBox msgBox;
    msgBox.setWindowTitle("Task submission not successful");
    msgBox.setText(errorText);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setWindowIcon(ORT_ICON);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
}


void ortMainWindow::showCloudProblem(QString text)
{
    QMessageBox msgBox(0);
    msgBox.setWindowTitle("YarraCloud");
    msgBox.setText(text+"<br><br>Do you want to review the configuration?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setWindowIcon(ORT_ICON);
    msgBox.setIcon(QMessageBox::Information);

    if (msgBox.exec() == QMessageBox::Yes)
    {
        // Call configuration dialog
        ortConfigurationDialog::executeDialog();
    }

    QTimer::singleShot(0, qApp, SLOT(quit()));
}


void ortMainWindow::on_logoLabel_customContextMenuRequested(const QPoint &pos)
{
    QString versionString="ORT Client Version ";
    versionString+=ORT_VERSION;

    QString serverString="Server:  " + modeList.serverName;

    if (modeList.serverName.isEmpty())
    {
        QString serverString="Server:  yarracloud.com";
    }

    QMenu infoMenu(this);
    infoMenu.addAction(versionString);
    infoMenu.addAction(serverString);
    infoMenu.addSeparator();
    infoMenu.addAction("Configuration...", this, SLOT(showConfiguration()));
    infoMenu.addAction("Show log file...", this, SLOT(showLogfile()));

    if (config.ortCloudSupportEnabled)
    {
        infoMenu.addAction("Show YarraCloud Agent...", this, SLOT(showYCAWindow()));
    }

    if (network.netLogger.isConfigured())
    {
        infoMenu.addSeparator();
        infoMenu.addAction("<Log Server Connected>");
    }
    else
    {
        if (network.netLogger.isConfigurationError())
        {
            infoMenu.addSeparator();
            infoMenu.addAction("<Error Connecting to Log Server>", this, SLOT(showLogfile()));
        }
    }

    infoMenu.exec(ui->logoLabel->mapToGlobal(pos));
}


void ortMainWindow::showLogfile()
{
    QString logPath=QDir::toNativeSeparators(RTI->getAppPath()+"/"+RTI->getLogInstance()->getLogFilename());
    RTI->log(logPath);

    RTI->flushLog();

    // Call notepad and show the log file
    QString cmdLine="notepad.exe";
    QStringList args;
    args.append(logPath);
    QProcess::startDetached(cmdLine, args);
}


void ortMainWindow::showConfiguration()
{
    hide();

    // Call configuration dialog
    if (ortConfigurationDialog::executeDialog())
    {        
        // Quit program is configuration has changed
        close();
    }
    else
    {
        show();
    }
}


void ortMainWindow::showYCAWindow()
{
    cloud.launchCloudAgent("show");
    close();
}


void ortMainWindow::on_manualAssignButton_clicked()
{
    isManualAssignment=ui->manualAssignButton->isChecked();
    ui->modeComboBox->setEnabled(isManualAssignment);
    ui->modeLabel->setEnabled(isManualAssignment);

    updateScanList();
}


void ortMainWindow::on_scansWidget_itemSelectionChanged()
{
    if (ui->scansWidget->selectedRanges().count()>0)
    {
        int selectedRow=ui->scansWidget->selectedRanges().at(0).topRow();
        if (selectedRow>=0)
        {
            int matchingMode=ui->scansWidget->item(selectedRow,6)->text().toInt();

            if ((matchingMode >= 0) && (matchingMode < ui->modeComboBox->maxCount()))
            {
                ui->modeComboBox->setCurrentIndex(matchingMode);
            }
        }
    }
}


void ortMainWindow::on_loadOlderButton_clicked()
{
    scansToShow=ORT_RAID_MAXPARSECOUNT;
    updateScanList();
}


void ortMainWindow::on_refreshButton_clicked()
{
    // Enforce new reading of raid list
    isRaidListAvaible=false;
    scansToShow=ORT_SCANSHOW_DEF;
    updateScanList();
}


void ortMainWindow::refreshRaidList()
{
    if (!raid.readRaidList())
    {
        RTI->log("Error reading the RAID list.");
        network.netLogger.postEvent(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error reading RAID list");
    }
    isRaidListAvaible=true;
}


void ortMainWindow::on_priorityButton_clicked(bool checked)
{
    // Change color of button if pressed
    if (checked)
    {
        QPalette pal=ui->priorityButton->palette();
        pal.setColor(QPalette::Button, QColor(255,106,19));
        pal.setColor(QPalette::ButtonText, QColor(255,255,255));

        ui->priorityButton->setPalette(pal);
    }
    else
    {
        ui->priorityButton->setPalette(this->palette());
    }
}


bool ortMainWindow::processCloudRecon(ortReconTask& task)
{
    // TODO: Add logserver events

    ortWaitDialog waitDialog;
    waitDialog.show();
    RTI->processEvents();

    task.cloudReconstruction=true;
    task.uuid=cloud.createUUID();

    if (!task.exportDataFiles(selectedFID, modeList.modes.at(selectedMode)))
    {
        //network.netLogger.postEventSync(EventInfo::Type::Transfer,EventInfo::Detail::Information,EventInfo::Severity::Error,"Error exporting file: "+reconTask.getErrorMessageUI());
        showTransferError(task.getErrorMessageUI());
        this->show();
        waitDialog.close();
        return false;
    }

    if (!task.anonymizeFiles())
    {
        showTransferError(task.getErrorMessageUI());
        this->show();
        waitDialog.close();
        return false;
    }

    if (!task.generateTaskFile())
    {
        showTransferError(task.getErrorMessageUI());
        this->show();
        waitDialog.close();
        return false;
    }

    cloud.launchCloudAgent("submit");

    waitDialog.close();
    return true;
}


