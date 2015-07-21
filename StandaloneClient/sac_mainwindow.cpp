#include "ui_sac_mainwindow.h"

#include <QtWidgets>
#include <QtCore>
#include "sac_mainwindow.h"

#include "sac_global.h"
#include "sac_bootdialog.h"
#include "sac_copydialog.h"
#include "sac_configurationdialog.h"


sacMainWindow::sacMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::sacMainWindow)
{
    restartApp=false;

    filename="";
    firstFileDialog=true;
    defaultMode=-1;
    scanFilename="";
    taskFilename="";
    lockFilename="";
    protocolName="";
    mode="";
    modeReadable="";
    notification="";
    scanFileSize=0;
    taskCreationTime=QDateTime::currentDateTime();
    paramValue=0;
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

        analyzeDatFile(filename, patientName, protocolName);
        if (patientName.length()>0)
        {
            ui->patnameEdit->setText(patientName);
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
            if (protocolName.length()>0)
            {
                initMode=detectMode(protocolName);
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

    patientName=ui->patnameEdit->text();
    accNumber=ui->accEdit->text();
    mode=modeList.modes.at(selectedIndex)->idName;
    modeReadable=modeList.modes.at(selectedIndex)->readableName;

    QString confirmText="Are you sure to submit the following reconstruction task?   \n\n";
    confirmText+="Patient: " + patientName + "\nACC#: ";
    if (accNumber.length()>0)
    {
        confirmText+=accNumber;
    }
    else
    {
        confirmText+="NONE";
    }
    confirmText+="\nMode: " + modeReadable + "\n";

    if (QMessageBox::question(this, "Confirm Submission", confirmText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)!=QMessageBox::Yes)
    {
        return;
    }

    this->hide();    
    RTI->processEvents();

    sacCopyDialog copyDialog;
    copyDialog.show();

    taskID=ui->taskIDEdit->text();

    // Remove space and other characters that might cause problems
    taskID.remove(QChar('.'),  Qt::CaseInsensitive);
    taskID.remove(QChar('/'),  Qt::CaseInsensitive);
    taskID.remove(QChar('\\'), Qt::CaseInsensitive);
    taskID.remove(QChar(':'),  Qt::CaseInsensitive);
    taskID.remove(QChar(' '),  Qt::CaseInsensitive);

    scanFilename=taskID;

    // Read the parameter value if the protocol uses one
    if (paramVisible)
    {
        paramValue=ui->paramEdit->text().toInt();

        if (paramValue>modeList.modes.at(selectedIndex)->paramMax)
        {
            paramValue=modeList.modes.at(selectedIndex)->paramMax;
        }
        if (paramValue<modeList.modes.at(selectedIndex)->paramMin)
        {
            paramValue=modeList.modes.at(selectedIndex)->paramMin;
        }

        scanFilename+=QString(RTI_SEPT_CHAR)+"P"+QString::number(paramValue);
    }
    else
    {
        paramValue=0;
    }

    taskFilename=scanFilename+ORT_TASK_EXTENSION;

    if (ui->priorityCombobox->currentIndex()==1)
    {
        taskFilename+=ORT_TASK_EXTENSION_NIGHT;
    }
    if (ui->priorityCombobox->currentIndex()==2)
    {
        taskFilename+=ORT_TASK_EXTENSION_PRIO;
    }

    lockFilename=scanFilename+ORT_LOCK_EXTENSION;
    scanFilename+=".dat";

    // First, get the notification addresses defined for the selected mode
    notification=modeList.modes.at(selectedIndex)->mailConfirmation;

    // Attach the entry from the dialog. Add separator character if needed
    QString mailRecipient=ui->notificationEdit->text();
    if ((notification!="") && (mailRecipient!=""))
    {
        notification+=",";
    }
    notification+=mailRecipient;

    scanFileSize=QFileInfo(filename).size();
    if (!network.copyMeasurementFile(filename,scanFilename))
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
        if (!generateTaskFile())
        {
            // Task-file creation failed, so remove scan file
            QFile::remove(network.serverDir.absoluteFilePath(scanFilename));

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


bool sacMainWindow::generateTaskFile()
{
    taskCreationTime=QDateTime::currentDateTime();

    QLockFile lockFile(network.serverDir.filePath(lockFilename));

    if (!lockFile.isLocked())
    {
        // Create write lock for the task file (to avoid that the server might
        // access the file is not entirely written)
        lockFile.lock();

        // Scoping for the lock file
        {
            // Create the task file
            QSettings taskFile(network.serverDir.filePath(taskFilename), QSettings::IniFormat);

            // Write the entries
            taskFile.setValue("Task/ReconMode", mode);
            taskFile.setValue("Task/ACC", accNumber);
            taskFile.setValue("Task/EMailNotification", notification);
            taskFile.setValue("Task/ScanFile", scanFilename);
            taskFile.setValue("Task/AdjustmentFilesCount", 0);
            taskFile.setValue("Task/PatientName", patientName);
            taskFile.setValue("Task/ScanProtocol", protocolName);
            taskFile.setValue("Task/ReconName", modeReadable);
            taskFile.setValue("Task/ParamValue", paramValue);

            taskFile.setValue("Information/SystemName", network.systemName);
            taskFile.setValue("Information/ScanFileSize", scanFileSize);
            taskFile.setValue("Information/TaskDate", taskCreationTime.date().toString(Qt::ISODate));
            taskFile.setValue("Information/TaskTime", taskCreationTime.time().toString(Qt::ISODate));
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
    infoMenu.addAction("Configuration...", this, SLOT(showConfiguration()));
    infoMenu.addAction("Show log file...", this, SLOT(showLogfile()));
    infoMenu.addSeparator();
    infoMenu.addAction(versionString);
    infoMenu.addAction(serverString);
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
    QString patID="<ParamString.\"tPatientName\">";

    detectedPatname="";
    detectedProtocol="";

    QFile file;
    file.setFileName(filename);
    file.open(QIODevice::ReadOnly);

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


