#include "ui_sac_mainwindow.h"

#include <QtWidgets>
#include <QtCore>

#include "sac_mainwindow.h"
#include "sac_global.h"
#include "sac_bootdialog.h"
#include "sac_copydialog.h"
#include "sac_configurationdialog.h"
#include "sac_batchdialog.h"
#include "sac_twixheader.h"
#include "ui_sac_batchdialog.h"


sacMainWindow::sacMainWindow(QWidget *parent, bool isConsole) :
    QMainWindow(parent),
    ui(new Ui::sacMainWindow),
    isConsole(isConsole)
{
    restartApp=false;

    didStart = false;
    filename="";
    firstFileDialog=true;
    defaultMode=-1;

    task.scanFilename="";
    task.taskFilename="";
    task.lockFilename="";
    task.protocolName="";
    task.mode="";
    task.modeReadable="";
    task.notification="";
    task.scanFileSize=0;
    task.taskCreationTime=QDateTime::currentDateTime();
    task.paramValue=0;
    paramVisible=false;

    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);    

    setWindowTitle("Yarra - Stand-Alone Client");
    setWindowIcon(SAC_ICON);

    log.start();
    RTI->setLogInstance(&log);

    cloud.setConfiguration(&cloudConfig);
    cloudConfig.loadConfiguration();

    if (!network.readConfiguration())
    {
        // Configuration is incomplete, so shut down
        if (! isConsole )
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Configuration Invalid");
            msgBox.setText("The Yarra stand-alone client has not been configured correctly.\n\nPlease check the configuration.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();

            QTimer::singleShot(0, this, SLOT(showFirstConfiguration()));
        }
        else
        {
            qInfo() << "Configuration Invalid";
            qInfo() << "The Yarra stand-alone client has not been configured correctly.\n\nPlease check the configuration.";
        }
        return;
    }

    sacBootDialog bootDialog;
    if (!isConsole)
    {
        bootDialog.show();
    }

    if ((network.serverPath.isEmpty()) && (network.cloudSupportEnabled))
    {
        // If no server path is given but cloud support is enabled, then
        // the user probably only wants to use cloud recon. In that case,
        // don't try to connect to avoid error messages
    }
    else
    {
        // If cloud support is off, or if a local server path has provided,
        // then try to connect to the server.

        if (!network.openConnection(isConsole))
        {
            if (network.showConfigurationAfterError)
            {
                QTimer::singleShot(0, this, SLOT(showFirstConfiguration()));
            }
            else
            {
                QTimer::singleShot(0, qApp, SLOT(quit()));
            }
            return;
        }

        modeList.network=&network;

        if (!modeList.readModeList())
        {
            QTimer::singleShot(0, qApp, SLOT(quit()));
            return;
        }
    }

    bool cloudModeLoaded=false;

    if (network.cloudSupportEnabled)
    {
        // Connect to the cloud service and read the list of cloud modes
        int cloudModes=cloud.readModeList(&modeList);

        if (cloudModes>0)
        {
            cloudModeLoaded=true;
        }
    }

    bootDialog.close();

    if (modeList.modes.count()==0)
    {
        ui->sendButton->setEnabled(false);
        ui->modeCombobox->setEnabled(false);
    }
    else
    {
        for (int i=0; i<modeList.modes.count(); i++)
        {
            ui->modeCombobox->addItem(modeList.modes.at(i)->readableName);

            // If at least one cloud mode has been loaded, add icons to
            // indicate where the modes are reconstructed
            if (cloudModeLoaded)
            {
                switch (modeList.modes.at(i)->computeMode)
                {
                default:
                case ortModeEntry::OnPremise:
                    ui->modeCombobox->setItemIcon(i,QIcon(":/images/premise.png"));
                    break;

                case ortModeEntry::Cloud:
                    ui->modeCombobox->setItemIcon(i,QIcon(":/images/cloud.png"));
                    break;

                case ortModeEntry::Elastic:
                    ui->modeCombobox->setItemIcon(i,QIcon(":/images/elastic.png"));
                    break;
                }
            }
        }
        on_modeCombobox_currentIndexChanged(0);

        findDefaultMode();
    }

    ui->notificationEdit->setText(network.defaultNotification);
    ui->priorityCombobox->setCurrentIndex(0);
    checkValues();

    returnFocusHelper.installOn(ui->accEdit);
    returnFocusHelper.installOn(ui->paramEdit);

    //QTimer::singleShot(0, this, SLOT(on_selectFileButton_clicked()));
    ui->selectFileButton->setFocus();
    didStart = true;
}


sacMainWindow::~sacMainWindow()
{
    if (network.cloudSupportEnabled)
    {
        cloud.launchCloudAgent();
    }

    network.closeConnection();

    RTI->setLogInstance(0);
    log.finish();

    delete ui;
}


void sacMainWindow::on_selectFileButton_clicked()
{
    QString newFilename=QFileDialog::getOpenFileName(this, "Select Measurement File...", QString(), "TWIX rawdata (*.dat)");
    QFileInfo fileInfo = QFileInfo(newFilename);

    if (newFilename.length()==0)
    {
        /*
        if (firstFileDialog)
        {
            close();
        }
        */
    }
    else
    {
        if (fileInfo.completeSuffix() == "sac")
        {
            auto reply = QMessageBox::question(this, "Test", "Submit this batch of raw files?",
                                               QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                handleBatchFile(fileInfo.absoluteFilePath());
            }
        }
        else
        {
            filename=newFilename;

            analyzeDatFile(filename, task.patientName, task.protocolName);
            if (task.patientName.length()>0)
            {
                ui->patnameEdit->setText(task.patientName);
            }

            int initMode=-1;
            // Check if a default mode has been defined. If so, use it to preselect the mode.
            if (defaultMode!=-1)
            {
                initMode=defaultMode;
                //on_modeCombobox_currentIndexChanged(defaultMode);
            }
            else
            {
                // Check if a protocol has been detected. If so, check if there is a
                // matching reconstruction mode. If so, preselect it.
                if (task.protocolName.length()>0)
                {
                    initMode=detectMode(task.protocolName);
                }
            }
            // If neither a default mode has been defined, nor a mode has been detected,
            // keep the mode as it is, otherwise change it.
            if (initMode!=-1)
            {
                ui->modeCombobox->setCurrentIndex(initMode);
            }

            QFileInfo fileinfo(filename);
            QString newtaskID=fileinfo.fileName();
            newtaskID.truncate(newtaskID.indexOf("."+fileinfo.completeSuffix()));
            ui->taskIDEdit->setText(newtaskID);
        }
    }

    ui->filenameEdit->setText(filename);
    // Clear the old ACC to avoid that an old ACC is used by mistake
    ui->accEdit->setText("");
    ui->priorityCombobox->setCurrentIndex(0);
    checkValues();
    firstFileDialog=false;
}


void sacMainWindow::on_cancelButton_clicked()
{
    close();
}


void sacMainWindow::on_sendButton_clicked()
{
    int selectedIndex=ui->modeCombobox->currentIndex();

    // If the patient name is empty
    if (ui->patnameEdit->text().length()==0)
    {
        ui->patnameEdit->setFocus();
        return;
    }

    // If ACC is required but not entered, highlight the control and return
    if ((modeList.modes.at(selectedIndex)->requiresACC) && (ui->accEdit->text().length()==0))
    {
        ui->accEdit->setFocus();
        return;
    }

    // If a parameter is requested, but the edit box empty, highlight the control and return
    if ((paramVisible) && (ui->paramEdit->text().length()==0))
    {
        ui->paramEdit->setFocus();
        return;
    }

    task.patientName=ui->patnameEdit->text();
    task.accNumber=ui->accEdit->text().toUpper();
    task.mode=modeList.modes.at(selectedIndex)->idName;
    task.modeReadable=modeList.modes.at(selectedIndex)->readableName;

    QString confirmText="Are you sure to submit the following task?   <br><br>";
    confirmText+="Patient: " + task.patientName + "<br>ACC#: ";
    if (task.accNumber.length()>0)
    {
        confirmText+=task.accNumber;
    }
    else
    {
        confirmText+="NONE";
    }
    confirmText+="<br>Mode: " + task.modeReadable + "<br>";

    if (modeList.modes.at(selectedIndex)->computeMode != ortModeEntry::OnPremise)
    {
        confirmText+="<br><strong>Note:</strong> You selected a YarraCloud-supported reconstruction.<br>The costs for the processing will be charged to your institution's credit card. Learn more about YarraCloud at <a href=\"http://yarracloud.com\"><span style=\"text-decoration: underline; color:#43b02a;\">http://yarracloud.com</span></a>.";
    }


    if (QMessageBox::question(this, "Confirm Submission", confirmText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)!=QMessageBox::Yes)
    {
        return;
    }

    this->hide();    
    RTI->processEvents();

    sacCopyDialog copyDialog;
    copyDialog.show();

    task.taskID=ui->taskIDEdit->text();

    // Remove space and other characters that might cause problems
    task.taskID.remove(QChar('.'),  Qt::CaseInsensitive);
    task.taskID.remove(QChar('/'),  Qt::CaseInsensitive);
    task.taskID.remove(QChar('\\'), Qt::CaseInsensitive);
    task.taskID.remove(QChar(':'),  Qt::CaseInsensitive);
    task.taskID.remove(QChar(' '),  Qt::CaseInsensitive);

    task.scanFilename=task.taskID;

    // Read the parameter value if the protocol uses one
    if (paramVisible)
    {
        task.paramValue=ui->paramEdit->text().toInt();

        if (task.paramValue>modeList.modes.at(selectedIndex)->paramMax)
        {
            task.paramValue=modeList.modes.at(selectedIndex)->paramMax;
        }
        if (task.paramValue<modeList.modes.at(selectedIndex)->paramMin)
        {
            task.paramValue=modeList.modes.at(selectedIndex)->paramMin;
        }

        task.scanFilename+=QString(RTI_SEPT_CHAR)+"P"+QString::number(task.paramValue);
    }
    else
    {
        task.paramValue=0;
    }

    task.taskFilename=task.scanFilename+ORT_TASK_EXTENSION;

    if (ui->priorityCombobox->currentIndex()==1)
    {
        task.taskFilename+=ORT_TASK_EXTENSION_NIGHT;
    }
    if (ui->priorityCombobox->currentIndex()==2)
    {
        task.taskFilename+=ORT_TASK_EXTENSION_PRIO;
    }

    task.lockFilename=task.scanFilename+ORT_LOCK_EXTENSION;
    task.scanFilename+=".dat";

    // First, get the notification addresses defined for the selected mode
    task.notification=modeList.modes.at(selectedIndex)->mailConfirmation;

    // Attach the entry from the dialog. Add separator character if needed
    QString mailRecipient=ui->notificationEdit->text();
    if ((task.notification!="") && (mailRecipient!=""))
    {
        task.notification+=",";
    }
    task.notification+=mailRecipient;

    task.scanFileSize=QFileInfo(filename).size();
    if (!network.copyMeasurementFile(filename,task.scanFilename))
    {
        copyDialog.close();
        this->show();
        this->activateWindow();
        RTI->processEvents();

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Transfer Error");
        msgBox.setText("The following problem occurred while transferring the data:\n\n" + network.copyErrorMsg + "\n\nThe reconstruction task will not be performed.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();       
    }
    else
    {
        if (!generateTaskFile(task))
        {
            // Task-file creation failed, so remove scan file
            QFile::remove(network.serverDir.absoluteFilePath(task.scanFilename));

            copyDialog.close();
            this->show();
            this->activateWindow();
            RTI->processEvents();

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Transfer Error");
            msgBox.setText("The task file could not be created on the server. Refer to the log file for further information.\n\nThe reconstruction task will not be performed.");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
        else
        {
            copyDialog.close();
            this->show();
            this->activateWindow();
            RTI->processEvents();

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Task Submitted");
            msgBox.setText("The reconstruction task has successfully been sent to the Yarra server.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
    }
}


bool sacMainWindow::generateTaskFile(Task &a_task)
{
    a_task.taskCreationTime=QDateTime::currentDateTime();

    QLockFile lockFile(network.serverDir.filePath(a_task.lockFilename));

    if (!lockFile.isLocked())
    {
        // Create write lock for the task file (to avoid that the server might
        // access the file is not entirely written)
        lockFile.lock();

        // Scoping for the lock file
        {
            // Create the task file
            QSettings taskFile(network.serverDir.filePath(a_task.taskFilename), QSettings::IniFormat);

            // Write the entries
            taskFile.setValue("Task/ReconMode", a_task.mode);
            taskFile.setValue("Task/ACC", a_task.accNumber);
            taskFile.setValue("Task/EMailNotification", a_task.notification);
            taskFile.setValue("Task/ScanFile", a_task.scanFilename);
            taskFile.setValue("Task/AdjustmentFilesCount", 0);
            taskFile.setValue("Task/PatientName", a_task.patientName);
            taskFile.setValue("Task/ScanProtocol", a_task.protocolName);
            taskFile.setValue("Task/ReconName", a_task.modeReadable);
            taskFile.setValue("Task/ParamValue", a_task.paramValue);

            taskFile.setValue("Information/SystemName", network.systemName);
            taskFile.setValue("Information/ScanFileSize", a_task.scanFileSize);
            taskFile.setValue("Information/TaskDate", a_task.taskCreationTime.date().toString(Qt::ISODate));
            taskFile.setValue("Information/TaskTime", a_task.taskCreationTime.time().toString(Qt::ISODate));
            taskFile.setValue("Information/SystemVendor",  "Siemens");
            taskFile.setValue("Information/SystemVersion", "Unknown");

            // Flush the entries into the file
            taskFile.sync();

            RTI->processEvents();
        }

        // Free the lock file
        lockFile.unlock();

        RTI->log("Generated task file. Done with submission.");
    }
    else
    {
        RTI->log("ERROR: Can't lock task file for submission.");
        return false;
    }

    return true;
}


void sacMainWindow::checkValues()
{
    bool canBeSent=false;

    if (filename.length()>0)
    {
        canBeSent=true;
    }

    if (modeList.modes.count()==0)
    {
        canBeSent=false;
    }

    ui->sendButton->setEnabled(canBeSent);
}


void sacMainWindow::on_modeCombobox_currentIndexChanged(int index)
{
    if (index==-1)
    {
        ui->paramEdit->setVisible(false);
        ui->paramLabel->setVisible(false);

        QFont accFont=ui->accLabel->font();
        accFont.setBold(false);
        ui->accLabel->setFont(accFont);
        return;
    }

    if (modeList.modes.at(index)->paramLabel.length()>0)
    {
        ui->paramEdit->setVisible(true);
        ui->paramLabel->setVisible(true);
        ui->paramLabel->setText(modeList.modes.at(index)->paramLabel+":");
        ui->paramEdit->setText(QString::number(modeList.modes.at(index)->paramDefault));
        paramVisible=true;
        updateDialogHeight();
    }
    else
    {
        ui->paramEdit->setVisible(false);
        ui->paramLabel->setVisible(false);
        paramVisible=false;
        updateDialogHeight();
    }

    QFont accFont=ui->accLabel->font();
    accFont.setBold(modeList.modes.at(index)->requiresACC);
    ui->accLabel->setFont(accFont);
}


void sacMainWindow::findDefaultMode()
{
    // If a preferred mode has been defined in the config file,
    // search for the index of this mode in the mode list and
    // remember the index.

    if (network.preferredMode.length()>0)
    {
        for (int i=0; i<modeList.modes.count(); i++)
        {
            if (modeList.modes.at(i)->idName==network.preferredMode)
            {
                defaultMode=i;
                break;
            }
        }
    }
}


void sacMainWindow::on_taskIDEdit_editingFinished()
{
    QString newTaskID=ui->taskIDEdit->text();

    newTaskID.remove(" ");
    newTaskID.remove("\\");
    newTaskID.remove("/");
    newTaskID.remove(":");
    newTaskID.remove(".");

    ui->taskIDEdit->setText(newTaskID);
}


void sacMainWindow::on_logoLabel_customContextMenuRequested(const QPoint &pos)
{
    QString versionString="SAC Client Version ";
    versionString+=SAC_VERSION;

    QString serverString="Server:  " + modeList.serverName;

    QMenu infoMenu(this);
    infoMenu.addAction(versionString);
    infoMenu.addAction(serverString);
    infoMenu.addSeparator();
    infoMenu.addAction("Configuration...", this, SLOT(showConfiguration()));
    infoMenu.addAction("Batch Processing...", this, SLOT(showBatchDialog()));

    infoMenu.addAction("Show log file...", this, SLOT(showLogfile()));
    infoMenu.exec(ui->logoLabel->mapToGlobal(pos));
}


void sacMainWindow::showLogfile()
{
    RTI->flushLog();

    // Call notepad and show the log file
    QString cmdLine="notepad.exe";
    QStringList args;
    args.append(qApp->applicationDirPath()+"/sac.log");
    QProcess::startDetached(cmdLine, args);
}


void sacMainWindow::showConfiguration()
{
    sacConfigurationDialog configurationDialog;
    configurationDialog.prepare(this);
    configurationDialog.exec();

    if (configurationDialog.closeMainWindow)
    {
        restartApp=true;
        close();
    }
}


void sacMainWindow::showFirstConfiguration()
{
    hide();
    sacConfigurationDialog configurationDialog;
    configurationDialog.prepare(this);
    configurationDialog.exec();

    if (configurationDialog.closeMainWindow)
    {
        restartApp=true;
    }
    close();
}


int sacMainWindow::detectMode(QString protocol)
{
    int detectedMode=-1;

    for (int i=0; i<modeList.modes.count(); i++)
    {
        if (protocol.contains(modeList.modes.at(i)->protocolTag))
        {
            detectedMode=i;
            break;
        }
    }
    return detectedMode;
}


void sacMainWindow::analyzeDatFile(QString filename, QString& detectedPatname, QString& detectedProtocol)
{
    QString protID="<ParamString.\"tProtocolName\">";
    QString patID ="<ParamString.\"tPatientName\">";

    detectedPatname ="";
    detectedProtocol="";

    QFile file;
    file.setFileName(filename);
    file.open(QIODevice::ReadOnly);

    // Determine whether file is VB or VD
    uint32_t x[2];
    file.read((char*)x, 2*sizeof(uint32_t));

    bool isVDVE=false;

    if ((x[0]==0) && (x[1]<=64))
    {
        isVDVE=true;
    }
    file.seek(0);

    // If file version is VD, then jump to the last measurement in the file
    if (isVDVE)
    {
        uint32_t id=0, ndset=0;
        std::vector<VD::EntryHeader> veh;

        file.read((char*)&id,   sizeof(uint32_t));  // ID
        file.read((char*)&ndset,sizeof(uint32_t));  // # data sets

        if (ndset>30)
        {
            // If there are more than 30 measurements, it's unlikely that the
            // file is a valid TWIX file
        }
        else
        {
            veh.resize(ndset);

            for (size_t i=0; i<ndset; ++i)
            {
                file.read((char*)&veh[i], VD::ENTRY_HEADER_LEN);
            }

            // Go to last measurement
            file.seek(veh.back().MeasOffset);
        }
    }

    bool patientFound=false;
    bool protocolFound=false;

    int linesToProcess=SAC_ANALYZE_MAXLINES;

    int idx=0;

    for (int i=0; i<linesToProcess; i++)
    {
        QByteArray line=file.readLine(SAC_ANALYZE_MAXLENGTH);

        if (!protocolFound)
        {
            idx=line.indexOf(protID);
            if (idx!=-1)
            {
                protocolFound=true;
                line.remove(idx, protID.length());
                line.remove(0, line.indexOf("\"")+1);
                line.truncate(line.indexOf("\""));
                detectedProtocol=QString(line);
                //RTI->log("Protocol: " + detectedProtocol);
            }
        }

        if (!patientFound)
        {
            idx=line.indexOf(patID);
            if (idx!=-1)
            {
                patientFound=true;
                line.remove(idx, protID.length());
                line.remove(0, line.indexOf("\"")+1);
                line.truncate(line.indexOf("\""));
                detectedPatname=QString(line);
                //RTI->log("Patient: " + detectedPatname);
            }
        }

        if (patientFound && protocolFound)
        {
            break;
        }
        if (file.atEnd())
        {
            break;
        }
    }

    if (!patientFound || !protocolFound)
    {
        RTI->log("WARNING: Could not find protocol or patient information.");
        RTI->log("WARNING: File " + filename);
    }

    file.close();
}


void sacMainWindow::updateDialogHeight()
{
    int newHeight=432;

    if (!paramVisible)
    {
        newHeight=402;
    }

    setMaximumHeight(newHeight);
    setMinimumHeight(newHeight);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(size().width(), newHeight), qApp->desktop()->availableGeometry()));
}


bool sacMainWindow::handleBatchFile(QString file)
{
    QSettings config(file, QSettings::IniFormat);

    int i=0;
    int j=0;

    while (1)
    {
        QString folder = QString("Scans/Folder") + QString::number(i);
        QString file   = QString("Scans/File") + QString::number(i);

        if (!config.contains(folder) || !config.contains(file))
        {
            break;
        }

        folder=config.value(folder,"" ).toString();
        file=config.value(file,"").toString();

        i++;
        j=0;

        while(1)
        {
            QString mode = QString("Modes/ReconMode") + QString::number(j);

            if (!config.contains(mode))
            {
             break;
            }

            mode = config.value(mode,"").toString();
            qInfo() << folder << file << mode;
            submitFileOfBatch(folder, file, mode, QString(), TaskPriority::Normal);
            j++;
        }
    }

    return true;
}


bool sacMainWindow::submitFileOfBatch(QString file_path, QString file_name, QString mode, QString notification, TaskPriority priority)
{
    Task the_task;
    QString the_file = QDir(file_path).filePath(file_name);

    analyzeDatFile(the_file, the_task.patientName, the_task.protocolName);

    the_task.mode = mode;

    QString modeNotification="";
    bool   foundMode;
    for (auto &modeInfo: modeList.modes)
    {
        if (modeInfo->idName == mode)
        {
            the_task.modeReadable = modeInfo->readableName;
            modeNotification=modeInfo->mailConfirmation;
            foundMode = true;
            break;
        }
    }

    if (!foundMode)
    {
        // Something's wrong- the mode id doesn't exist.
        qInfo() << mode << " is not available, or doesn't exist, on this server";
        return false;
    }

    // Combine notification addresses from UI and mode definiton
    the_task.notification = notification;

    if (!modeNotification.isEmpty())
    {
        if (!the_task.notification.isEmpty())
        {
            the_task.notification += ",";
        }
        the_task.notification += modeNotification;
    }

    QFileInfo fileinfo(the_file);


    the_task.scanFilename = file_name;

    // Check if the file already exists on the server. If so, add a timestamp and check again.
    // Do this 5 times. If that can't resolve it, then go on and fail with error message during
    // the copy procedure.
    for (int i=0; i<5; i++)
    {
        if (network.fileExistsOnServer(the_task.scanFilename))
        {
            QFileInfo newFile(the_task.scanFilename);
            QString timestamp="_" + QTime::currentTime().toString("hhmmsszzz");
            the_task.scanFilename=newFile.completeBaseName() + timestamp + "." + newFile.suffix();

            RTI->log("File exists on server. Adding timestamp: " + the_task.scanFilename);
        }
        else
        {
            break;
        }
    }

    the_task.scanFileSize = fileinfo.size();
    the_task.taskCreationTime=QDateTime::currentDateTime();
    the_task.taskFilename=the_task.scanFilename+ORT_TASK_EXTENSION;
    the_task.accNumber="0";
    the_task.paramValue=0;

    if (priority == TaskPriority::Night)
    {
        task.taskFilename+=ORT_TASK_EXTENSION_NIGHT;
    }
    else
    {
        if (priority == TaskPriority::HighPriority)
        {
            task.taskFilename+=ORT_TASK_EXTENSION_PRIO;
        }
    }

    QString newtaskID=fileinfo.fileName();
    newtaskID.truncate(newtaskID.indexOf("."+fileinfo.completeSuffix()));

    the_task.taskID=newtaskID;
    the_task.lockFilename=task.scanFilename+ORT_LOCK_EXTENSION;

    sacCopyDialog copyDialog;
    copyDialog.show();

    if (!network.copyMeasurementFile(the_file,the_task.scanFilename))
    {
        RTI->log("ERROR: Unable to copy file " + the_file);
        copyDialog.hide();
        return false;
    }

    copyDialog.hide();

    if (!generateTaskFile(the_task))
    {        
        RTI->log("ERROR: Unable to generate task file for " + the_file);

        // Task-file creation failed, so remove scan file
        QFile::remove(network.serverDir.absoluteFilePath(the_task.scanFilename));
        return false;
    }

    return true;
}


void sacMainWindow::showBatchDialog()
{
    sacBatchDialog batchDialog(this);
    batchDialog.prepare(modeList.modes, ui->notificationEdit->text(), defaultMode);

    this->hide();

    while(true)
    {
        bool exit   = true;
        int  result = batchDialog.exec();

        if (result == QDialog::Rejected)
        {
            break;
        }

        if (batchDialog.modes->rowCount() == 0 || batchDialog.files->rowCount() == 0)
        {
            QMessageBox::information(this, "Error", "You must select both files and modes.", QMessageBox::Ok);
            continue;
        }

        for (QString mode: batchDialog.modes->stringList())
        {
            bool foundMode = false;

            for (auto &modeInfo: modeList.modes)
            {
                if (modeInfo->idName == mode )
                {
                    foundMode = true;
                    break;
                }
            }

            if (!foundMode)
            {
                auto reply = QMessageBox::question(this, "Warning", "Mode " + mode + " does not exist on this server. This reconstruction will fail. Continue?",
                                                QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::No)
                {
                    exit=false;
                }
                break;
            }
        }

        if (!exit)
        {
            continue;
        }

        TaskPriority priority = TaskPriority::Normal;

        if (ui->priorityCombobox->currentIndex()==1)
        {
            priority = TaskPriority::Night;
        }
        if (ui->priorityCombobox->currentIndex()==2)
        {
            priority = TaskPriority::HighPriority;
        }      

        if (submitBatch(batchDialog.files->stringList(), batchDialog.modes->stringList(), batchDialog.ui->notificationEdit->text(), priority))
        {
            QMessageBox::information(this, "Batch submitted", "Successfully submitted batch task to server.", QMessageBox::Ok);
        }

        break;
    }

    this->show();
}


bool sacMainWindow::submitBatch(QStringList files, QStringList modes, QString notify, TaskPriority priority)
{
    for (QString file: files)
    {
        QFileInfo fi(file);

        for (QString mode: modes)
        {
            while (!submitFileOfBatch(fi.absolutePath(), fi.fileName(), mode, notify, priority))
            {
                int reply;
                if (!isConsole)
                {
                    reply = QMessageBox::question(this, "Transfer Error", "Submitting " + file + " for mode < " + mode + " > failed. Retry?",
                                                  QMessageBox::Yes|QMessageBox::Ignore|QMessageBox::Abort);
                }

                if(reply == QMessageBox::Abort || isConsole)
                {
                    RTI->log("ERROR: Submitting file failed " + file + " for mode " + mode);
                    qCritical() << "Submitting " << file << " < " << mode << " > failed.";
                    return false;
                }
                else
                {
                    if (reply == QMessageBox::Ignore)
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
    }

    return true;
}


bool sacMainWindow::readBatchFile(QString fileName, QStringList& files, QStringList& modes, QString& notify, TaskPriority& priority)
{
    QFileInfo fileInfo = QFileInfo(fileName);
    if (! fileInfo.exists() )
    {
        RTI->log("ERROR: Batch file not found");
        qInfo() << "ERROR: Batch file not found";
        return false;
    }

    QSettings batchInfo(fileInfo.absoluteFilePath(), QSettings::IniFormat);

    notify = batchInfo.value("Info/Notify","").toString();

    QString priority_s = batchInfo.value("Info/Priority","").toString();
    if (priority_s == "Normal")       priority = TaskPriority::Normal;
    if (priority_s == "Night")        priority = TaskPriority::Night;
    if (priority_s == "HighPriority") priority = TaskPriority::HighPriority;

    int i=0;
    while (1)
    {
        QString scan = QString("Scans/Scan") + QString::number(i);

        if (!batchInfo.contains(scan))
        {
            break;
        }

        scan = batchInfo.value(scan,"" ).toString();
        files.append(scan);
        i++;
    }

    i=0;
    while (1)
    {
        QString mode = QString("ReconModes/Mode") + QString::number(i);

        if (!batchInfo.contains(mode))
        {
            break;
        }

        mode=batchInfo.value(mode,"" ).toString();
        modes.append(mode);
        i++;
    }

    return true;
}
