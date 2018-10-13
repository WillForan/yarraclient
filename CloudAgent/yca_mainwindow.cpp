#include "yca_mainwindow.h"
#include "yca_global.h"
#include "ui_yca_mainwindow.h"

#include <QDesktopWidget>
#include <QMessageBox>
#include <QTest>

#include "../CloudTools/yct_common.h"
#include "../CloudTools/yct_aws/qtaws.h"


ycaWorker::ycaWorker()
{
    //qInfo() << "Main Thread: " << QThread::currentThreadId();

    processingActive=false;
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
}


void ycaWorker::stopTimer()
{
    transferTimer.stop();
    transferThread.quit();
}


void ycaWorker::timerCall()
{
    //qInfo() << "Called. Thread: " << QThread::currentThreadId();

    processingActive=true;
    transferTimer.stop();

    if (parent->taskHelper.removeIncompleteDownloads())
    {
        // TODO: Error handing
    }

    ycaTaskList taskList;
    parent->taskHelper.getScheduledTasks(taskList);

    if (!taskList.empty())
    {
        if (!parent->cloud.validateUser(&transferInformation))
        {
            if (!userInvalidShown)
            {
                QMetaObject::invokeMethod(parent, "showNotification", Qt::QueuedConnection,  Q_ARG(QString, "Cannot process cases. User account is missing payment information or has been diabled."));

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
    parent->taskHelper.getJobsForDownloadArchive(taskList, jobsToDownload, jobsToArchive);

    if (!jobsToDownload.empty())
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

            if (!parent->cloud.downloadCase(currentTask, &transferInformation, &parent->mutex))
            {
                // TODO: Error handling
            }

            parent->mutex.lock();
            currentTaskID="";
            parent->mutex.unlock();
        }

        QMetaObject::invokeMethod(parent, "hideIndicator", Qt::QueuedConnection);
    }

    // TODO: Push downloaded jobs to destination
    // TODO: Retrieve storage location for each job
    currentProcess=ycaTask::wpStorage;
    updateParentStatus();

    ycaTaskList jobsToStore;


    if (!jobsToArchive.empty())
    {
        parent->mutex.lock();
        if (!parent->taskHelper.archiveJobs(jobsToArchive))
        {
            // TODO: Error handling
        }
        parent->mutex.unlock();
    }

    currentProcess=ycaTask::wpIdle;
    updateParentStatus();
    transferTimer.start();
    processingActive=false;

    if (parent->isVisible())
    {
        QMetaObject::invokeMethod(parent, "updateUI", Qt::QueuedConnection);
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

    if (!cloud.createCloudFolders())
    {
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
            qApp->quit();
        }
    }
    else
    {
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
    infoMenu.addAction("Restart", this,  SLOT(callShutDown()));
    infoMenu.addAction("Shutdown", this, SLOT(callShutDown()));
    infoMenu.exec(ui->closeContextButton->mapToGlobal(QPoint(0,0)));
}


void ycaMainWindow::on_statusRefreshButton_clicked()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    mutex.lock();
    taskHelper.getAllTasks(taskList, true, false);
    mutex.unlock();

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


void ycaMainWindow::on_pushButton_3_clicked()
{
    indicator.hideIndicator();
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


void ycaMainWindow::on_pushButton_4_clicked()
{
    cloud.getJobStatus(&taskList);
}
