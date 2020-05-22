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

    for (int i=0; i<int(rdsRuntimeInformation::RDS_SYNGOVERSIONS_COUNT); i++)
    {
        ui->syngoBox->addItem(RTI->getSyngoMRVersionString(i));
    }
    ui->syngoBox->setCurrentIndex(int(rdsRuntimeInformation::RDS_VE11C));

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);
}


rdsDebugWindow::~rdsDebugWindow()
{
    delete ui;
}


void rdsDebugWindow::on_setDebugButton_clicked()
{
    RTI->setDebugMode(true);
    RTI->debug("Debug mode enabled");
}


void rdsDebugWindow::on_saveRaidListButton_clicked()
{
    ui->textEdit->append("Reading RAID and parsing directory...");
    RTI_RAID->readRaidList();
    RTI_RAID->createExportList();
    RTI_RAID->dumpRaidList("raid_debug.txt");
    ui->textEdit->append("Done.");
    ui->textEdit->append("Output path: " + RTI->getAppPath() + "/raid_debug.txt");
}


void rdsDebugWindow::on_saveRaidOutputButton_clicked()
{
    ui->textEdit->append("Reading RAID directory...");
    RTI_RAID->readRaidList();
    RTI_RAID->dumpRaidToolOutput("raid_dump.txt");
    ui->textEdit->append("Done");
    ui->textEdit->append("Output path: " + RTI->getAppPath() + "/raid_dump.txt");
}


void rdsDebugWindow::on_testFileButton_clicked()
{
    QString filename=QFileDialog::getOpenFileName(Q_NULLPTR, "Select Test File", "", "Test Files (*.txt)");
    ui->testFileEdit->setText(filename);
}


void rdsDebugWindow::on_parserTestButton_clicked()
{
    RTI->setDebugMode(true);

    // Switch the syngo version according to the format of the selected file
    RTI->debugPatchSyngoVersion(ui->syngoBox->currentIndex());

    QString filename=ui->testFileEdit->text();
    bool result=RTI->getRaidInstance()->debugReadTestFile(filename);

    ui->textEdit->append("Saving raid list after parsing file.");
    RTI_RAID->dumpRaidList("raid_debug.txt");
    ui->textEdit->append("Output path: " + RTI->getAppPath() + "/raid_debug.txt");

    if (result)
    {
        QMessageBox::information(this, "Success", "Parsing the provided file was successful.");
    }
    else
    {
        QMessageBox::critical(this, "Error", "Parsing the provided file was not successful. Check error logs.");
    }

    QMessageBox::information(this, "Restart required", "You MUST restart the RDS client after finishing the test.", QMessageBox::Ok, QMessageBox::Ok);
}


void rdsDebugWindow::on_testAnonymizerButton_clicked()
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
