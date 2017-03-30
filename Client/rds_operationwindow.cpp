#include "rds_operationwindow.h"
#include "rds_configurationwindow.h"
#include "ui_rds_operationwindow.h"

#include "rds_global.h"


rdsOperationWindow::rdsOperationWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsOperationWindow)
{
    ui->setupUi(this);
    log.setLogWidget(ui->logEdit);

    trayIconMenu = new QMenu(this);
    trayItemTransferNow=trayIconMenu->addAction("Transfer data now",this, SLOT(callManualUpdate()));
    trayIconMenu->addSeparator();
    trayIconMenu->addAction("Show status window",this, SLOT(showNormal()));
    trayItemConfiguration=trayIconMenu->addAction("Configuration",this, SLOT(callConfiguration()));
    trayIconMenu->addSeparator();
    trayItemShutdown=trayIconMenu->addAction("Shut down service",this, SLOT(callShutDown()));

    QIcon icon = RDS_ICON;
    setWindowIcon(icon);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(icon);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip("Yarra Data Transfer Client");
    trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    connect(ui->shutdownButton, SIGNAL(clicked()), this, SLOT(callShutDown()));
    connect(ui->configurationButton, SIGNAL(clicked()), this, SLOT(callConfiguration()));
    connect(ui->hideButton, SIGNAL(clicked()), this, SLOT(hide()));
    connect(ui->logfileButton, SIGNAL(clicked()), this, SLOT(callShowLogfile()));
    connect(ui->transferButton, SIGNAL(clicked()), this, SLOT(callManualUpdate()));
    connect(ui->postponeButton, SIGNAL(clicked()), this, SLOT(callPostpone()));

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    setUIModeIdle();

    config.loadConfiguration();   

    if (!config.isConfigurationValid())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Configuration invalid");
        msgBox.setText("The service has not been configured correctly. Please validate the configuration and restart the service.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(RDS_ICON);
        msgBox.exec();

        QTimer::singleShot(0, this, SLOT(callConfiguration()));
    }
    else
    {
        log.start();
        RTI->setLogInstance(&log);
        RTI->setConfigInstance(&config);
        RTI->setNetworkInstance(&network);
        RTI->setRaidInstance(&raid);
        RTI->setControlInstance(&control);
        RTI->setWindowInstance(this);

        RTI_NETLOG.configure(RTI_CONFIG->logServerPath, EventInfo::SourceType::RDS,RTI_CONFIG->infoName,RTI_CONFIG->logServerKey);

        // Notify the process controller about the start of the service
        control.setStartTime();
        updateInfoUI();

        RTI_NETLOG.postEvent(EventInfo::Type::Boot,EventInfo::Detail::Information,EventInfo::Severity::Success,"startup");

        // Start the timer for triggering updates. Checks update condition only every
        // minute to prevent undesired system load
        controlTimer.setInterval(RDS_TIMERINTERVAL);
        connect(&controlTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
        controlTimer.start();
    }

    if (raid.isPatchedRaidToolMissing())
    {
        RTI->log("ERROR: Patched Raid Tool for VD13 is missing.");
        RTI->log("ERROR: Shutting down service.");
        QTimer::singleShot(0, this, SLOT(callImmediateShutdown()));
    }
}


rdsOperationWindow::~rdsOperationWindow()
{
    RTI->setLogInstance(0);
    log.finish();
    delete ui;
}


void rdsOperationWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
}


void rdsOperationWindow::keyPressEvent(QKeyEvent* event)
{
    if ((event->key()==Qt::Key_D) && (event->modifiers()==Qt::ControlModifier))
    {
        QInputDialog dbgDialog;
        dbgDialog.setInputMode(QInputDialog::TextInput);
        dbgDialog.setWindowIcon(RDS_ICON);
        dbgDialog.setWindowTitle("Password");
        dbgDialog.setLabelText("Debug Password:");
        dbgDialog.setTextEchoMode(QLineEdit::Password);

        if ((dbgDialog.exec()!=QDialog::Rejected) && (dbgDialog.textValue()==RDS_DBGPASSWORD))
        {
            debugWindow.show();
        }
    }

    QDialog::keyReleaseEvent(event);
}


void rdsOperationWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
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


void rdsOperationWindow::callShutDown()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Shut down service?");
    msgBox.setText("Do you really want to shut down the service? Raw data will not be transfered and might be lost if the service is inactive for longer time.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setWindowIcon(RDS_ICON);
    int ret = msgBox.exec();

    if (ret==QMessageBox::Yes)
    {
        log.log("Shut down.");
        RTI->setMode(rdsRuntimeInformation::RDS_QUIT);
        qApp->quit();
    }
}


void rdsOperationWindow::callImmediateShutdown()
{
    log.log("Immediate shut down.");
    log.flush();
    RTI->setMode(rdsRuntimeInformation::RDS_QUIT);
    qApp->quit();
}


void rdsOperationWindow::callConfiguration()
{
    if (rdsConfigurationWindow::checkAccessPassword())
    {
        RTI->setMode(rdsRuntimeInformation::RDS_CONFIGURATION);
        qApp->quit();
    }
    else
    {
        // If the configuration password is wrong and the system has not
        // been configured yet, then shut down the program. Otherwise,
        // the user would end up in an infinite loop.
        if (!config.isConfigurationValid())
        {
            RTI->setMode(rdsRuntimeInformation::RDS_QUIT);
            qApp->quit();
        }
    }
}


void rdsOperationWindow::callShowLogfile()
{
    RTI->flushLog();

    // Call notepad and show the log file
    QString cmdLine="notepad.exe";
    QStringList args;
    args.append(RTI->getAppPath()+"/"+RTI->getLogInstance()->getLogFilename());

    QProcess::startDetached(cmdLine, args);
}


void rdsOperationWindow::setUIModeProgress()
{
    ui->transferButton->setEnabled(false);
    ui->logfileButton->setEnabled(false);
    ui->configurationButton->setEnabled(false);
    ui->shutdownButton->setEnabled(false);

    ui->postponeButton->setEnabled(true);

    // Disable items in the context menu of the tray icon
    trayItemTransferNow->setEnabled(false);
    trayItemConfiguration->setEnabled(false);
    trayItemShutdown->setEnabled(false);
}


void rdsOperationWindow::setUIModeIdle()
{
    ui->transferButton->setEnabled(true);
    ui->logfileButton->setEnabled(true);
    ui->configurationButton->setEnabled(true);
    ui->shutdownButton->setEnabled(true);

    ui->postponeButton->setEnabled(false);

    // Enable items in the context menu of the tray icon
    trayItemTransferNow->setEnabled(true);
    trayItemConfiguration->setEnabled(true);
    trayItemShutdown->setEnabled(true);
}


void rdsOperationWindow::checkForUpdate()
{
    // Stop the timer, ask the controller to check
    // if an update is needed. If so, switch UI
    // control to update mode, perform the update,
    // switch UI back and start the timer again.
    controlTimer.stop();

    if (control.isUpdateNeeded())
    {
        setUIModeProgress();
        control.setActivityWindow(this->isHidden());
        control.performUpdate();
        setUIModeIdle();
    }

    controlTimer.start();
}


void rdsOperationWindow::callManualUpdate()
{
    if (control.getState()==rdsProcessControl::STATE_IDLE)
    {
        controlTimer.stop();

        setUIModeProgress();
        control.setActivityWindow(this->isHidden());
        control.performUpdate();
        setUIModeIdle();

        controlTimer.start();
    }
}


void rdsOperationWindow::callPostpone()
{
    RTI->setPostponementRequest(true);
}


void rdsOperationWindow::updateInfoUI()
{
    ui->InfoValueSystem->setText(RTI_CONFIG->infoName);

    if (RTI_CONTROL->getState()==RTI_CONTROL->STATE_IDLE)
    {
        if (RTI_CONTROL->isWaitingFirstUpdate())
        {
            ui->InfoValueUpdate->setText("Waiting");
        }
        else
        {
            ui->InfoValueUpdate->setText(RTI_CONTROL->getLastUpdateTime().toString());
        }

        QFont f=ui->InfoValueError->font();
        f.setPointSize(8);
        ui->InfoValueError->setFont(f);

        if (RTI->isSevereErrors())
        {
            ui->InfoValueError->setText("<b>Severe errors occured during update approach.</b>");
        }
        else
        {
            ui->InfoValueError->setText("");
        }
    }
    else
    {
        QFont f=ui->InfoValueError->font();
        f.setPointSize(10);
        ui->InfoValueError->setFont(f);

        ui->InfoValueUpdate->setText("Active");

        if (RTI_CONTROL->getState()==RTI_CONTROL->STATE_NETWORKTRANSFER)
        {
            ui->InfoValueError->setText("<span style=""color:#580F8B;""><b>Transfer running...<br>It is safe to scan now.</b></span>");
            //hide();
        }
        else
        {
            ui->InfoValueError->setText("<span style=""color:#580F8B;""><b>Update running...<br>Don't start new scans at this time!</b></b></span>");
        }
    }
}
