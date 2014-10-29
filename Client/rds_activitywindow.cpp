#include "rds_activitywindow.h"
#include "ui_rds_activitywindow.h"

#include "rds_global.h"
#include "rds_processcontrol.h"


rdsActivityWindow::rdsActivityWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsActivityWindow)
{
    ui->setupUi(this);

    QIcon icon = RDS_ICON;
    setWindowIcon(icon);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::WindowStaysOnTopHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    connect(ui->postponeButton, SIGNAL(clicked()), this, SLOT(callPostponement()));
}


rdsActivityWindow::~rdsActivityWindow()
{
    delete ui;
}


void rdsActivityWindow::callPostponement()
{
    RTI->setPostponementRequest(true);
}


