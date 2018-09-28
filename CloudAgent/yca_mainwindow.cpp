#include "yca_mainwindow.h"
#include "yca_global.h"

#include "ui_yca_mainwindow.h"

#include <QDesktopWidget>
#include <QMessageBox>

#include "../CloudTools/yct_common.h"
#include "../CloudTools/yct_aws/qtaws.h"



ycaWorker::ycaWorker()
{
    moveToThread(&transferThread);
    transferTimer.moveToThread(&transferThread);
    transferThread.start();
    QMetaObject::invokeMethod(this, "startTimer", Qt::QueuedConnection);
}


void ycaWorker::shutdown()
{
    QMetaObject::invokeMethod(this, "stopTimer", Qt::QueuedConnection);
}


void ycaWorker::startTimer()
{
    this->connect(&transferTimer, SIGNAL(timeout()), SLOT(timerCall()), Qt::DirectConnection);
    transferTimer.setInterval(1000);
    transferTimer.start();
}


void ycaWorker::stopTimer()
{
    transferTimer.stop();
    transferThread.quit();
}


void ycaWorker::timerCall()
{
    transferTimer.stop();
    qInfo() << "Called";
    transferTimer.start();
}


ycaMainWindow::ycaMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ycaMainWindow)
{
    ui->setupUi(this);

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

    ui->versionLabel->setText("Version " + QString(YCA_VERSION) + ", Build date " + QString(__DATE__));

    // Center the window on the screen
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,size(),
                                    qApp->desktop()->availableGeometry()));

    indicator.setMainDialog((QDialog*) this);
    config.loadConfiguration();

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


void ycaMainWindow::callSubmit()
{
    indicator.showIndicator();
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
    QtAWSRequest awsRequest(config.key, config.secret);
    QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/modes",
                                            QByteArray(), YCT_API_REGION, QByteArray(), QStringList());

    ui->activeTasksTable->setRowCount(1);
    ui->activeTasksTable->setColumnCount(1);

    if (!reply.isSuccess())
    {
        ui->activeTasksTable->setItem(0,0,new QTableWidgetItem(QString("Error")));
    }
    else
    {
        ui->activeTasksTable->setItem(0,0,new QTableWidgetItem(QString(reply.replyData())));
    }

    QApplication::restoreOverrideCursor();



    //config.loadConfiguration();
    /*
    config.key="Test-Key";
    config.secret="Test-Secret";
    config.saveConfiguration();
    */

    /*
    ui->activeTasksTable->setRowCount(1);
    ui->activeTasksTable->setColumnCount(3);

    ui->activeTasksTable->setItem(0,0,new QTableWidgetItem(config.key));
    ui->activeTasksTable->setItem(0,1,new QTableWidgetItem(config.secret));
    ui->activeTasksTable->setItem(0,2,new QTableWidgetItem(config.getRegion()));
    */
}


void ycaMainWindow::on_pushButton_5_clicked()
{
    indicator.showIndicator();
}


void ycaMainWindow::on_pushButton_3_clicked()
{
    indicator.hideIndicator();
    trayIcon->showMessage("YarraCloud", "Case has been uploaded\nCase ID: 4IKX8", QSystemTrayIcon::NoIcon);
}
