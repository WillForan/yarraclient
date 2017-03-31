#include <QDesktopWidget>

#include "rds_iconwindow.h"
#include "ui_rds_iconwindow.h"

#include "rds_global.h"


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

    error=false;
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


