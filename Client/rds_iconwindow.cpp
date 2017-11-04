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

    // Menubar of Syngo MR is 25 pixels high
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignTop, QSize(32,25), qApp->desktop()->availableGeometry()));

    showStartupCommandsEntry=false;
    error=false;
}


void rdsIconWindow::showStartupCommandsOption()
{
    showStartupCommandsEntry=true;
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
        infoMenu.addAction("Yarra RDS Client - Ver " + QString(RDS_VERSION));
        infoMenu.addSeparator();
        infoMenu.addAction("Show Status Window...",this,SLOT(showStatusWindow()));
        if (showStartupCommandsEntry)
        {
            infoMenu.addAction("Run Startup Commands",this,SLOT(runStartupCommands()));
        }
        infoMenu.exec(this->mapToGlobal((event->pos())));
    }
}


void rdsIconWindow::showStatusWindow()
{
    RTI->showOperationWindow();
    clearError();
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


