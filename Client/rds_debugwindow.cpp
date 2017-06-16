#include "rds_debugwindow.h"
#include "ui_rds_debugwindow.h"

#include "rds_global.h"
#include "rds_processcontrol.h"
#include "rds_raid.h"
#include "rds_anonymizeVB17.h"


rdsDebugWindow::rdsDebugWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsDebugWindow)
{
    ui->setupUi(this);

    setWindowIcon(RDS_ICON);
}



rdsDebugWindow::~rdsDebugWindow()
{
    delete ui;
}

void rdsDebugWindow::on_pushButton_2_clicked()
{
    RTI->setDebugMode(true);
    RTI->debug("Debug mode enabled");
}

void rdsDebugWindow::on_pushButton_clicked()
{
    ui->textEdit->append("Reading RAID and parsing directory...");
    RTI_RAID->readRaidList();
    RTI_RAID->createExportList();
    RTI_RAID->dumpRaidList();
    ui->textEdit->append("Done.");
    ui->textEdit->append("Output path: " + RTI->getAppPath() + "/debug.txt");
}

void rdsDebugWindow::on_pushButton_3_clicked()
{
    ui->textEdit->append("Reading RAID directory...");
    RTI_RAID->readRaidList();
    RTI_RAID->dumpRaidToolOutput();
    ui->textEdit->append("Done");
    ui->textEdit->append("Output path: " + RTI->getAppPath() + "/debug.txt");
}

void rdsDebugWindow::on_pushButton_4_clicked()
{
    QString filename="";

    filename=QFileDialog::getOpenFileName(this, "Select VB17 File", "", "TWIX VB17 (*.dat)");

    if (filename!="")
    {
        if (!rdsAnonymizeVB17::anonymize(filename))
        {
            RTI->log("ERROR from anonymization tool!");
        }
        else
        {
            RTI->log("Anonymization successful.");
        }
    }
}
