#include "ui_sac_mainwindow.h"

#include <QtWidgets>
#include <QtCore>

#include "sac_mainwindow.h"
#include "sac_global.h"
#include "sac_bootdialog.h"
#include "sac_copydialog.h"
#include "sac_configurationdialog.h"
#include "sac_twixheader.h"

bool sacMainWindow::batchSubmit(QString file_path, QString file_name, QString mode ){
    Task the_task;
    QString the_file = QDir(file_path).filePath(file_name);
    analyzeDatFile(the_file, the_task.patientName, the_task.protocolName);
    the_task.mode = mode;
    bool foundMode;
    for (auto &modeInfo: modeList.modes) {
        if (modeInfo->idName == mode ) {
            the_task.modeReadable = modeInfo->readableName;
            foundMode = true;
            break;
        }
    }

    if (!foundMode)
    { // Something's wrong- the mode id doesn't exist.
        qInfo() << mode << " is not available, or doesn't exist, on this server";
        return false;
    }

    the_task.accNumber="0";
    the_task.notification = network.defaultNotification;

    QFileInfo fileinfo(the_file);
    the_task.scanFilename = file_name;
    the_task.scanFileSize = fileinfo.size();
    the_task.taskCreationTime=QDateTime::currentDateTime();
    the_task.taskFilename=the_task.scanFilename+ORT_TASK_EXTENSION;
    QString newtaskID=fileinfo.fileName();
    newtaskID.truncate(newtaskID.indexOf("."+fileinfo.completeSuffix()));

    the_task.taskID=newtaskID;
    the_task.lockFilename=task.scanFilename+ORT_LOCK_EXTENSION;

    if (!network.copyMeasurementFile(the_file,the_task.scanFilename)){
        return false;
    }

    if (!generateTaskFile(the_task))
    {
        // Task-file creation failed, so remove scan file
        QFile::remove(network.serverDir.absoluteFilePath(the_task.scanFilename));
        return false;
    }
    return true;
}

sacMainWindow::sacMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::sacMainWindow)
{
    restartApp=false;

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

    if (!network.readConfiguration())
    {
        // Configuration is incomplete, so shut down
        QMessageBox msgBox;
        msgBox.setWindowTitle("Configuration Invalid");
        msgBox.setText("The Yarra stand-alone client has not been configured correctly.\n\nPlease check the configuration.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        QTimer::singleShot(0, this, SLOT(showFirstConfiguration()));
        return;
    }

    sacBootDialog bootDialog;
    bootDialog.show();

    if (!network.openConnection())
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

    bootDialog.close();


    auto appPath=qApp->applicationDirPath();

    {
        QSettings config(appPath+"/test.ini", QSettings::IniFormat);

        int i=0;
        int j=0;
        while (1)
        {
            QString folder = QString("Scans/Folder") + QString::number(i);
            QString file = QString("Scans/File") + QString::number(i);

            if (!config.contains(folder) || !config.contains(file))
                break;

            folder=config.value(folder,"" ).toString();
            file=config.value(file,"").toString();
            i++;
            j=0;
            while(1)
            {
                QString mode = QString("Modes/ReconMode") + QString::number(j);
                if (!config.contains(mode)){
                 break;
                }
                mode = config.value(mode,"").toString();
                qInfo() << folder << file << mode;
                batchSubmit(folder,file,mode);
                j++;
            }
        }
    }



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
}


sacMainWindow::~sacMainWindow()
{
    network.closeConnection();

    RTI->setLogInstance(0);
    log.finish();

    delete ui;
}


void sacMainWindow::on_selectFileButton_clicked()
{
    QString newFilename=QFileDialog::getOpenFileName(this, "Select Measurement File...", QString(), "TWIX rawdata (*.dat)");

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

    QString confirmText="Are you sure to submit the following reconstruction task?   \n\n";
    confirmText+="Patient: " + task.patientName + "\nACC#: ";
    if (task.accNumber.length()>0)
    {
        confirmText+=task.accNumber;
    }
    else
    {
        confirmText+="NONE";
    }
    confirmText+="\nMode: " + task.modeReadable + "\n";

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
