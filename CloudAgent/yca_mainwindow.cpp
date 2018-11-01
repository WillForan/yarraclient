#include "yca_mainwindow.h"
#include "yca_global.h"
#include "ui_yca_mainwindow.h"
#include "yca_threadlog.h"

#include <QDesktopWidget>
#include <QMessageBox>

#include "../CloudTools/yct_common.h"
#include "../CloudTools/yct_aws/qtaws.h"


ycaWorker::ycaWorker()
{
    processingActive=false;
    processingPaused=false;
    currentProcess=ycaTask::wpIdle;
    currentTaskID="";
    userInvalidShown=false;

    moveToThread(&transferThread);
    transferTimer.moveToThread(&transferThread);
    transferThread.start();
}


void ycaWorker::setParent(ycaMainWindow* myParent)
{
    parent=myParent;
    QMetaObject::invokeMethod(this, "startTimer", Qt::QueuedConnection);
}


void ycaWorker::shutdown()
{
    QMetaObject::invokeMethod(this, "stopTimer", Qt::QueuedConnection);
}


void ycaWorker::trigger()
{
    if (!processingActive)
    {
        QMetaObject::invokeMethod(this, "timerCall", Qt::QueuedConnection);
    }
}


void ycaWorker::startTimer()
{
    this->connect(&transferTimer, SIGNAL(timeout()), SLOT(timerCall()), Qt::DirectConnection);
    transferTimer.setInterval(60000);
    transferTimer.start();
    updateParentStatus();

    YTL->log("Worker thread started",YTL_INFO,YTL_LOW);
}


void ycaWorker::stopTimer()
{
    transferTimer.stop();
    transferThread.quit();
}


void ycaWorker::timerCall()
{
    if (processingPaused)
    {
        return;
    }

    processingActive=true;
    transferTimer.stop();

    YTL->log("Started worker update",YTL_INFO,YTL_MID);

    if (!parent->taskHelper.removeIncompleteDownloads())
    {
        YTL->log("Error while removing incomplete downloads",YTL_ERROR,YTL_HIGH);
        // TODO: Error handing
    }

    ycaTaskList taskList;
    parent->taskHelper.getScheduledTasks(taskList);

    if (!taskList.empty())
    {
        if (!parent->cloud.validateUser(&transferInformation))
        {
            YTL->log("Unable to validate user account",YTL_ERROR,YTL_HIGH);

            if (!userInvalidShown)
            {
                QMetaObject::invokeMethod(parent, "showNotification", Qt::QueuedConnection,  Q_ARG(QString, "Cannot process cases. User account is missing payment information or has been disabled."));

                // Avoid that the message is shown during every timer event
                userInvalidShown=true;
            }
        }
        else
        {
            QMetaObject::invokeMethod(parent, "showIndicator", Qt::QueuedConnection);

            currentProcess=ycaTask::wpUpload;
            updateParentStatus();

            // Only upload 10 cases per time, then first download the recon
            int uploadCount=0;

            while ((!taskList.isEmpty()) && (uploadCount<10))
            {
                parent->mutex.lock();
                currentTaskID=taskList.first()->uuid;
                parent->mutex.unlock();

                if (!parent->cloud.uploadCase(taskList.takeFirst(), &transferInformation, &parent->mutex))
                {
                    YTL->log("Error while uploading case "+currentTaskID,YTL_ERROR,YTL_HIGH);
                    // TODO: Error handling
                }
                uploadCount++;

                parent->mutex.lock();
                currentTaskID="";
                parent->mutex.unlock();
            }

            QMetaObject::invokeMethod(parent, "hideIndicator", Qt::QueuedConnection);
        }
    }

    parent->mutex.lock();
    parent->taskHelper.getProcessingTasks(taskList);
    parent->cloud.getJobStatus(&taskList);
    // TODO: Get error status
    parent->mutex.unlock();

    ycaTaskList jobsToArchive;
    ycaTaskList jobsToDownload;
    parent->taskHelper.getTasksForDownloadArchive(taskList, jobsToDownload, jobsToArchive);

    // Both the failed and successful jobs should now have the costs set,
    // so save the information to the PHI file
    parent->mutex.lock();
    parent->taskHelper.saveCostsToPHI(jobsToDownload);
    parent->taskHelper.saveCostsToPHI(jobsToArchive);
    parent->mutex.unlock();

    if (!jobsToDownload.empty())
    {
        if (transferInformation.username.isEmpty())
        {
            if (!parent->cloud.validateUser(&transferInformation))
            {
                YTL->log("Unable to validate user account",YTL_ERROR,YTL_HIGH);

                if (!userInvalidShown)
                {
                    QMetaObject::invokeMethod(parent, "showNotification", Qt::QueuedConnection,  Q_ARG(QString, "Cannot process cases. User account is missing payment information or has been disabled."));

                    // Avoid that the message is shown during every timer event
                    userInvalidShown=true;
                }
            }
        }

        if (transferInformation.userAllowed)
        {
            QMetaObject::invokeMethod(parent, "showIndicator", Qt::QueuedConnection);

            currentProcess=ycaTask::wpDownload;
            updateParentStatus();

            while (!jobsToDownload.isEmpty())
            {
                parent->mutex.lock();
                ycaTask* currentTask=jobsToDownload.takeFirst();
                currentTaskID=currentTask->uuid;
                parent->mutex.unlock();

                qDebug() << "Now downloading...";

                if (!parent->cloud.downloadCase(currentTask, &transferInformation, &parent->mutex))
                {
                    YTL->log("Error while downloading case "+currentTaskID,YTL_ERROR,YTL_HIGH);

                    // TODO: Error handling
                }

                parent->mutex.lock();
                currentTaskID="";
                parent->mutex.unlock();
            }

            // Don't hide the indicator here, because if jobs have been downloaded,
            // a storage operation will follow
        }
    }

    currentProcess=ycaTask::wpStorage;
    updateParentStatus();  

    // Retrieve storage location for each job and push downloaded jobs to destination
    if (!parent->taskHelper.storeTasks(jobsToArchive,parent))
    {
        YTL->log("Error while storing cases",YTL_ERROR,YTL_HIGH);
        // TODO: Error handling
    }

    if (!jobsToArchive.empty())
    {
        QString successNotification="";

        parent->mutex.lock();
        if (!parent->taskHelper.archiveTasks(jobsToArchive,successNotification))
        {
            YTL->log("Error while archiving tasks",YTL_ERROR,YTL_HIGH);
            // TODO: Error handling
        }
        parent->mutex.unlock();

        if ((parent->config.showNotifications) && (!successNotification.isEmpty()))
        {
            successNotification="Tasks have been completed: \n"+successNotification;
            QMetaObject::invokeMethod(parent, "showNotification", Qt::QueuedConnection, Q_ARG(QString, successNotification));
        }
    }

    QMetaObject::invokeMethod(parent, "hideIndicator", Qt::QueuedConnection);

    YTL->log("Completed worker update",YTL_INFO,YTL_MID);

    currentProcess=ycaTask::wpIdle;
    updateParentStatus();
    transferTimer.start();
    processingActive=false;

    if (parent->isVisible())
    {
        QMetaObject::invokeMethod(parent, "updateUIWorker", Qt::QueuedConnection);
    }
}


void ycaWorker::updateParentStatus()
{
    QString text="Unknown";

    switch (currentProcess)
    {
    default:
    case ycaTask::wpIdle:
        text="<span style=\" font-weight:600; color:#E0A526;\">Idle</span>";
        break;
    case ycaTask::wpUpload:
        text="<span style=\" font-weight:600; color:#580f8b;\">Uploading tasks...</span>";
        break;
    case ycaTask::wpDownload:
        text="<span style=\" font-weight:600; color:#580f8b;\">Downloading results...</span>";
        break;
    case ycaTask::wpStorage:
        text="<span style=\" font-weight:600; color:#580f8b;\">Storing results...</span>";
        break;
    }

    QMetaObject::invokeMethod(parent, "showStatus", Qt::QueuedConnection,  Q_ARG(QString, text));
}


ycaMainWindow::ycaMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ycaMainWindow)
{
    YTL->getInstance();

    ui->setupUi(this);
    shuttingDown=false;

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction("YarraCloud Agent v" + QString(YCA_VERSION));
    trayIconMenu->addSeparator();
    trayIconMenu->addAction("Show window",this, SLOT(showNormal()));
    trayItemShutdown=trayIconMenu->addAction("Shutdown",this, SLOT(callShutDown()));

    QIcon icon = YCA_ICON;
    setWindowIcon(icon);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(icon);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip("YarraCloud Agent");
    trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(show()));

    ui->versionLabel->setText("Version " + QString(YCA_VERSION) + ", Build date " + QString(__DATE__));

    // Center the window on the screen
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,size(),
                                    qApp->desktop()->availableGeometry()));

    indicator.setMainDialog((QDialog*) this);

    cloud.setConfiguration(&config);
    config.loadConfiguration();
    taskHelper.setCloudInstance(&cloud);

    ui->notificationsCheckbox->setChecked(config.showNotifications);

    if (!cloud.createCloudFolders())
    {
        YTL->log("Unable to create cloud folders",YTL_ERROR,YTL_HIGH);

        QMessageBox msgBox(0);
        msgBox.setWindowTitle("Yarra Cloud Agent");
        msgBox.setText("Unable to prepare folder for cloud reconstruction.<br>Please check you local client installation.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(YCA_ICON);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();

        transferWorker.shutdown();
        shuttingDown=true;
        QTimer::singleShot(0, qApp, SLOT(quit()));
        return;
    }

    QDir appDir(qApp->applicationDirPath());
    if (!appDir.exists("YCA_helper.exe"))
    {
        YTL->log("Unable to locate helper app",YTL_ERROR,YTL_HIGH);

        QMessageBox msgBox(0);
        msgBox.setWindowTitle("Yarra Cloud Agent");
        msgBox.setText("Unable to find folder application YCA_helper.exe.<br>Please check you local client installation.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(YCA_ICON);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();

        transferWorker.shutdown();
        shuttingDown=true;
        QTimer::singleShot(0, qApp, SLOT(quit()));
        return;
    }

    if (!checkForDCMTK())
    {
        YTL->log("Unable to locate DCMTK binaries",YTL_ERROR,YTL_HIGH);

        QMessageBox msgBox(0);
        msgBox.setWindowTitle("Yarra Cloud Agent");
        msgBox.setText("Unable to locate DCMTK binaries.<br>Please check you local client installation.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(YCA_ICON);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();

        transferWorker.shutdown();
        shuttingDown=true;
        QTimer::singleShot(0, qApp, SLOT(quit()));
        return;
    }

    transferWorker.setParent(this);
    taskHelper.clearTaskList(taskList);

    ui->activeTasksTable->setColumnCount(5);
    ui->activeTasksTable->setHorizontalHeaderItem(0,new QTableWidgetItem("Status"));
    ui->activeTasksTable->setHorizontalHeaderItem(1,new QTableWidgetItem("Patient"));
    ui->activeTasksTable->setHorizontalHeaderItem(2,new QTableWidgetItem("ACC"));
    ui->activeTasksTable->setHorizontalHeaderItem(3,new QTableWidgetItem("MRN"));
    ui->activeTasksTable->setHorizontalHeaderItem(4,new QTableWidgetItem("ID"));
    ui->activeTasksTable->horizontalHeader()->resizeSection(0,130);
    ui->activeTasksTable->horizontalHeader()->resizeSection(1,220);
    ui->activeTasksTable->horizontalHeader()->resizeSection(2,90);
    ui->activeTasksTable->horizontalHeader()->resizeSection(3,90);

    ui->archiveTasksTable->setColumnCount(6);
    ui->archiveTasksTable->setHorizontalHeaderItem(0,new QTableWidgetItem("Status"));
    ui->archiveTasksTable->setHorizontalHeaderItem(1,new QTableWidgetItem("Patient"));
    ui->archiveTasksTable->setHorizontalHeaderItem(2,new QTableWidgetItem("ACC"));
    ui->archiveTasksTable->setHorizontalHeaderItem(3,new QTableWidgetItem("MRN"));
    ui->archiveTasksTable->setHorizontalHeaderItem(4,new QTableWidgetItem("Cost"));
    ui->archiveTasksTable->setHorizontalHeaderItem(5,new QTableWidgetItem("ID"));
    ui->archiveTasksTable->horizontalHeader()->resizeSection(0,130);
    ui->archiveTasksTable->horizontalHeader()->resizeSection(1,220);
    ui->archiveTasksTable->horizontalHeader()->resizeSection(2,90);
    ui->archiveTasksTable->horizontalHeader()->resizeSection(3,90);
    ui->archiveTasksTable->horizontalHeader()->resizeSection(4,90);

    ui->searchTable->setColumnCount(5);
    ui->searchTable->setHorizontalHeaderItem(0,new QTableWidgetItem("Status"));
    ui->searchTable->setHorizontalHeaderItem(1,new QTableWidgetItem("Patient"));
    ui->searchTable->setHorizontalHeaderItem(2,new QTableWidgetItem("ACC"));
    ui->searchTable->setHorizontalHeaderItem(3,new QTableWidgetItem("MRN"));
    ui->searchTable->setHorizontalHeaderItem(4,new QTableWidgetItem("ID"));
    ui->searchTable->horizontalHeader()->resizeSection(0,130);
    ui->searchTable->horizontalHeader()->resizeSection(1,220);
    ui->searchTable->horizontalHeader()->resizeSection(2,90);
    ui->searchTable->horizontalHeader()->resizeSection(3,90);

    YTL->log("UI thread running",YTL_INFO,YTL_LOW);
}


ycaMainWindow::~ycaMainWindow()
{
    delete ui;
}


void ycaMainWindow::closeEvent(QCloseEvent* event)
{
    if (trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
}


void ycaMainWindow::callShutDown(bool askConfirmation)
{
    // TODO: Check if there are pending uploads/downloads

    if (askConfirmation)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Shutdown Agent?");
        msgBox.setText("Do you really want to shutdown the agent? Outgoing jobs and finished reconstructions will not be transfered anymore.");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setWindowIcon(YCA_ICON);
        int ret = msgBox.exec();

        if (ret==QMessageBox::Yes)
        {
            transferWorker.shutdown();
            YTL->log("Shutdown (user request)",YTL_INFO,YTL_HIGH);
            qApp->quit();
        }
    }
    else
    {
        YTL->log("Shutdown",YTL_INFO,YTL_HIGH);
        qApp->quit();
    }
}


void ycaMainWindow::showIndicator()
{
    indicator.showIndicator();
}


void ycaMainWindow::hideIndicator()
{
    indicator.hideIndicator();
}


void ycaMainWindow::callSubmit()
{
    transferWorker.trigger();
}


void ycaMainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        showNormal();
        break;
    default:
        break;
    }
}


void ycaMainWindow::on_closeButton_clicked()
{
    hide();
}


void ycaMainWindow::on_closeContextButton_clicked()
{
    QMenu infoMenu(this);
    infoMenu.addAction("Shutdown", this, SLOT(callShutDown()));
    infoMenu.exec(ui->closeContextButton->mapToGlobal(QPoint(0,0)));
}


void ycaMainWindow::on_statusRefreshButton_clicked()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    YTL->log("Refreshing UI status",YTL_INFO,YTL_LOW);

    mutex.lock();
    taskHelper.getAllTasks(taskList, true, false);
    mutex.unlock();

    ui->activeTasksTable->clearContents();
    qApp->processEvents();
    ui->activeTasksTable->setRowCount(taskList.count());

    QTableWidgetItem* item=0;
    for (int i=0; i<taskList.count(); i++)
    {
        item=new QTableWidgetItem(taskList.at(i)->getStatus());
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->activeTasksTable->setItem(i,0,item);

        item=new QTableWidgetItem(taskList.at(i)->patientName);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->activeTasksTable->setItem(i,1,item);

        item=new QTableWidgetItem(taskList.at(i)->acc);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->activeTasksTable->setItem(i,2,item);

        item=new QTableWidgetItem(taskList.at(i)->mrn);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->activeTasksTable->setItem(i,3,item);

        item=new QTableWidgetItem(taskList.at(i)->uuid);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->activeTasksTable->setItem(i,4,item);
    }

    if (ui->activeTasksTable->rowCount()>0)
    {
        int colCount=ui->activeTasksTable->columnCount()-1;
        ui->activeTasksTable->setRangeSelected(QTableWidgetSelectionRange(0,0,0,colCount),true);
    }

    QApplication::restoreOverrideCursor();
}


void ycaMainWindow::showNotification(QString text)
{
    trayIcon->showMessage("YarraCloud", text, QSystemTrayIcon::NoIcon, 20000);
}


void ycaMainWindow::showStatus(QString text)
{
    ui->statusLabel->setText(text);
}


void ycaMainWindow::on_detailsButton_clicked()
{
    if (ui->activeTasksTable->selectedRanges().empty())
    {
        return;
    }

    int selectedTask=ui->activeTasksTable->selectedRanges().at(0).topRow();

    if ((selectedTask>=0) && (selectedTask<taskList.count()))
    {
        ycaTask* taskItem=taskList.at(selectedTask);

        QString infoText="Patient: " + taskItem->patientName +"<br>";

        QMessageBox msgBox(0);
        msgBox.setWindowTitle("Task Information");
        msgBox.setText(infoText);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(YCA_ICON);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }
}


void ycaMainWindow::on_tabWidget_currentChanged(int index)
{
    // Update the active jobs list when changing ot the tab
    if (index==0)
    {
        on_statusRefreshButton_clicked();
    }

    if (index==1)
    {
        on_refreshArchiveButton_clicked();
    }

    if (index==2)
    {
        ui->searchEdit->setFocus();
    }

    if (index==3)
    {
        on_refreshLogButton_clicked();
    }
    else
    {
        ui->logWidget->clearContents();
    }
}


void ycaMainWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    qApp->processEvents();
    QTimer::singleShot(0,this,SLOT(updateUI()));
}


void ycaMainWindow::updateUI()
{
    qApp->processEvents();    
    on_tabWidget_currentChanged(ui->tabWidget->currentIndex());
}


void ycaMainWindow::updateUIWorker()
{
    qApp->processEvents();

    // If the update is triggered by the worker thread, don't update
    // the log tab (to avoid that the scrollbar jumps back)
    if (ui->tabWidget->currentIndex()!=3)
    {
        on_tabWidget_currentChanged(ui->tabWidget->currentIndex());
    }
}


void ycaMainWindow::on_transferButton_clicked()
{
    YTL->log("Transfer triggered by user",YTL_INFO,YTL_LOW);
    callSubmit();
}


void ycaMainWindow::on_pauseButton_clicked()
{
    if (ui->pauseButton->isChecked())
    {
        YTL->log("Updates paused by user",YTL_INFO,YTL_LOW);
        transferWorker.processingPaused=true;
    }
    else
    {
        YTL->log("Updates resumed by user",YTL_INFO,YTL_LOW);
        transferWorker.processingPaused=false;
    }
}


void ycaMainWindow::on_clearArchiveButton_clicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Clear Tasks?");
    msgBox.setText("Do you really want to clear the list of completed / failed tasks? The task information will be deleted and cannot be recovered.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setWindowIcon(YCA_ICON);
    int ret = msgBox.exec();

    if (ret==QMessageBox::Yes)
    {
        // TODO
    }
}


void ycaMainWindow::on_notificationsCheckbox_clicked()
{
    config.showNotifications=ui->notificationsCheckbox->isChecked();
    config.saveConfiguration();
}


void ycaMainWindow::on_refreshArchiveButton_clicked()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    mutex.lock();
    taskHelper.getAllTasks(archiveList, false, true);
    mutex.unlock();

    ui->archiveTasksTable->clearContents();
    qApp->processEvents();
    ui->archiveTasksTable->setRowCount(archiveList.count());

    QTableWidgetItem* item=0;
    for (int i=0; i<archiveList.count(); i++)
    {
        item=new QTableWidgetItem(archiveList.at(i)->getResult());
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->archiveTasksTable->setItem(i,0,item);

        item=new QTableWidgetItem(archiveList.at(i)->patientName);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->archiveTasksTable->setItem(i,1,item);

        item=new QTableWidgetItem(archiveList.at(i)->acc);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->archiveTasksTable->setItem(i,2,item);

        item=new QTableWidgetItem(archiveList.at(i)->mrn);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->archiveTasksTable->setItem(i,3,item);

        item=new QTableWidgetItem("$ "+QString::number(archiveList.at(i)->cost,'f',2));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->archiveTasksTable->setItem(i,4,item);

        item=new QTableWidgetItem(archiveList.at(i)->uuid);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->archiveTasksTable->setItem(i,5,item);
    }

    if (ui->archiveTasksTable->rowCount()>0)
    {
        int colCount=ui->archiveTasksTable->columnCount()-1;
        ui->archiveTasksTable->setRangeSelected(QTableWidgetSelectionRange(0,0,0,colCount),true);
    }

    QApplication::restoreOverrideCursor();
}


void ycaMainWindow::on_archiveDetailsButton_clicked()
{
    if (ui->archiveTasksTable->selectedRanges().empty())
    {
        return;
    }
}


void ycaMainWindow::on_searchButton_clicked()
{
    ui->searchTable->clearContents();
    taskHelper.clearTaskList(taskList);

    ycaTaskList searchList;
    taskHelper.getAllTasks(searchList,true,true);

    while (!searchList.empty())
    {
        ycaTask* task=searchList.takeFirst();
        if ((!ui->searchEdit->text().isEmpty()) && (task->patientName.contains(ui->searchEdit->text(),Qt::CaseInsensitive)))
        {
            taskList.append(task);
        }
        else
        {
            delete task;
            task=0;
        }
    }

    if (taskList.empty())
    {
        ui->searchTable->setRowCount(1);

        QTableWidgetItem* item=new QTableWidgetItem("Nothing found");
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->searchTable->setItem(0,0,item);
        ui->searchTable->setItem(0,1,new QTableWidgetItem(""));
        ui->searchTable->setItem(0,2,new QTableWidgetItem(""));
        ui->searchTable->setItem(0,3,new QTableWidgetItem(""));
        ui->searchTable->setItem(0,4,new QTableWidgetItem(""));
    }
    else
    {
        ui->searchTable->setRowCount(taskList.count());
        QTableWidgetItem* item=0;
        for (int i=0; i<taskList.count(); i++)
        {
            item=new QTableWidgetItem(taskList.at(i)->getStatus());
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui->searchTable->setItem(i,0,item);

            item=new QTableWidgetItem(taskList.at(i)->patientName);
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            ui->searchTable->setItem(i,1,item);

            item=new QTableWidgetItem(taskList.at(i)->acc);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui->searchTable->setItem(i,2,item);

            item=new QTableWidgetItem(taskList.at(i)->mrn);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui->searchTable->setItem(i,3,item);

            item=new QTableWidgetItem(taskList.at(i)->uuid);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui->searchTable->setItem(i,4,item);
        }
    }
}


void ycaMainWindow::on_searchEdit_returnPressed()
{
    on_searchButton_clicked();
}


void ycaMainWindow::on_detailsSearchButton_clicked()
{
    // TODO
}


bool ycaMainWindow::checkForDCMTK()
{
    QDir appDir(qApp->applicationDirPath());

    if (!appDir.exists(YCT_DCMTK_DCMODIFY))
    {
        return false;
    }

    if (!appDir.exists(YCT_DCMTK_STORESCU))
    {
        return false;
    }

    if (   (!appDir.exists(YCT_DCMTK_COPYRIGHT))
        || (!appDir.exists(YCT_DCMTK_SUPPORT1 ))
        || (!appDir.exists(YCT_DCMTK_SUPPORT2 ))
        || (!appDir.exists(YCT_DCMTK_SUPPORT3 ))
        || (!appDir.exists(YCT_DCMTK_SUPPORT4 )))
    {
        return false;
    }

    return true;
}


void ycaMainWindow::on_refreshLogButton_clicked()
{
    int detailLevel=ui->logDetailCombobox->currentIndex();
    YTL->readLogFile(ui->logWidget,detailLevel);
    ui->logWidget->scrollToTop();
}


void ycaMainWindow::on_logDetailCombobox_currentIndexChanged(int index)
{
    on_refreshLogButton_clicked();
}


void ycaMainWindow::on_externalLogButton_clicked()
{
    QString logPath=qApp->applicationDirPath()+"/log/yca.log";

    // Call notepad and show the log file
    QString cmdLine="notepad.exe";
    QStringList args;
    args.append(logPath);
    QProcess::startDetached(cmdLine, args);
}


void ycaMainWindow::on_logTicketButton_clicked()
{
    QMessageBox msgBox(0);
    msgBox.setWindowTitle("Coming Soon");
    msgBox.setText("Feature will be implemented soon.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setWindowIcon(YCA_ICON);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}


void ycaMainWindow::on_logClipboardButton_clicked()
{
    QClipboard* clipboard=QApplication::clipboard();
    QString copyText=YTL->getClipboardString(ui->logWidget);
    clipboard->setText(copyText);
}

