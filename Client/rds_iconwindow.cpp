#include <QDesktopWidget>
#include <QMenu>

#include "rds_iconwindow.h"
#include "ui_rds_iconwindow.h"
#include "rds_global.h"
#include "rds_operationwindow.h"


// Note: For the animation to work, qgif.dll from the QT plugin folder has to be put in C:\yarra\imageformats

rdsIconWindow::rdsIconWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsIconWindow)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowStaysOnTopHint;
    flags |= Qt::Tool;
    setWindowFlags(flags);

    QPalette p = palette();
    p.setColor(QPalette::Background, QColor(0,0,0));
    this->setPalette(p);

    animRunning=false;
    anim=new QMovie(":/images/topIcon_run.gif");
    ui->animIconLabel->setMovie(anim);
    ui->animIconLabel->setVisible(false);
    ui->errorIconLabel->setVisible(false);
    ui->staticIconLabel->setVisible(true);

    if (RTI->isSyngoXALine())
    {
        // For NumarixX, place the icon in the status bar
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignHCenter | Qt::AlignBottom, QSize(32,25), qApp->desktop()->screenGeometry()));
    }
    else
    {
        // Menubar of Syngo MR is 25 pixels high
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignTop, QSize(32,25), qApp->desktop()->availableGeometry()));
    }

    showStartupCommandsEntry=false;
    showCloudWindowEntry=false;
    showORTEntry=false;
    error=false;
}


void rdsIconWindow::showStartupCommandsOption()
{
    showStartupCommandsEntry=true;
}


void rdsIconWindow::showCloudWindowOption()
{
    showCloudWindowEntry=true;
}


rdsIconWindow::~rdsIconWindow()
{
    delete ui;
}


void rdsIconWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    RTI->showOperationWindow();    
    clearError();
}


void rdsIconWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button()==Qt::RightButton)
    {
        QMenu infoMenu(this);
        infoMenu.setStyle(new rdsIconProxyStyle());
        infoMenu.addAction("Yarra RDS Client - Version " + QString(RDS_VERSION));
        infoMenu.addSeparator();

        if (showStartupCommandsEntry)
        {
            QIcon icon = CMD_ICON_MENU;
            infoMenu.addAction(icon,"Run Startup Commands",this,SLOT(runStartupCommands()));
        }

        {
            QIcon icon = RDS_ICON;
            infoMenu.addAction(icon,"Transfer Data Now",this,SLOT(triggerTransferNow()));
        }

        {
            QIcon icon = RDS_ICON;
            infoMenu.addAction(icon,"Show Status Window...",this,SLOT(showStatusWindow()));
        }

        if (showCloudWindowEntry)
        {
            QIcon icon = YCA_ICON_MENU;
            infoMenu.addAction(icon,"Show YarraCloud Agent...",this,SLOT(showCloudAgent()));
        }
        infoMenu.addSeparator();

        if (showORTEntry)
        {
            QIcon icon = ORT_ICON_MENU;
            infoMenu.addAction(icon,"Offline Reconstruction Task...",this,SLOT(startORTClient()));
        }

        infoMenu.exec(this->mapToGlobal((event->pos())));
    }
}


void rdsIconWindow::showStatusWindow()
{
    RTI->showOperationWindow();
    clearError();
}


void rdsIconWindow::showCloudAgent()
{
    QString cmd=qApp->applicationDirPath() + "/YCA.exe show";
    QProcess::startDetached(cmd);
}


void rdsIconWindow::startORTClient()
{
    QString cmd=qApp->applicationDirPath() + "/ORT.exe";
    QProcess::startDetached(cmd);
}


void rdsIconWindow::runStartupCommands()
{
    if (RTI->getControlInstance()->getState()==rdsProcessControl::STATE_IDLE)
    {
        RTI->log("User requet to rerun startup commands.");
        RTI->getWindowInstance()->runStartCmds();
    }
}


void rdsIconWindow::setAnim(bool value)
{
    if (animRunning)
    {
        animRunning=false;
        anim->stop();       
        ui->animIconLabel->setVisible(false);

        if (error)
        {
            ui->errorIconLabel->setVisible(true);
        }
        else
        {
            ui->staticIconLabel->setVisible(true);
        }
    }
    else
    {
        animRunning=true;
        anim->start();

        ui->staticIconLabel->setVisible(false);
        ui->errorIconLabel ->setVisible(false);
        ui->animIconLabel  ->setVisible(true);
    }
}


void rdsIconWindow::clearError()
{
    error=false;

    if (ui->errorIconLabel->isVisible())
    {
        ui->errorIconLabel->setVisible(false);
        ui->staticIconLabel->setVisible(true);
    }
}


void rdsIconWindow::setError()
{
    error=true;

    if (!animRunning)
    {
        ui->errorIconLabel->setVisible(true);
        ui->staticIconLabel->setVisible(false);
    }
}


void rdsIconWindow::showORTOption()
{
    showORTEntry=true;
}


void rdsIconWindow::triggerTransferNow()
{
    RTI->getWindowInstance()->triggerManualUpdate();
}
