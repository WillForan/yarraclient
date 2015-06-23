#include "rds_configurationwindow.h"
#include "ui_rds_configurationwindow.h"

#include "rds_global.h"
#include <QtWidgets>


rdsConfigurationWindow::rdsConfigurationWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsConfigurationWindow)
{
    ui->setupUi(this);
    setWindowIcon(RDS_ICON);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(saveSettings()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    connect(ui->updateCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(callUpdateModeChanged(int)));

    connect(ui->protAddButton, SIGNAL(clicked()), this, SLOT(callAddProt()));
    connect(ui->protRemoveButton, SIGNAL(clicked()), this, SLOT(callRemoveProt()));

    //connect(ui->protListWidget, SIGNAL(activated()), this, SLOT(callShowProt()));


    ui->tabWidget->setCurrentIndex(0);
    callUpdateModeChanged(ui->updateCombobox->currentIndex());

    ui->networkModeCombobox->setCurrentIndex(0);
    ui->networkStackedWidget->setCurrentIndex(0);

    ui->versionLabel->setText("Version " + QString(RDS_VERSION) + ", Build date " + QString(__DATE__));

    readConfiguration();

    ui->ortConnectEdit->setToolTip("Enter the full command-shell command for mapping the share from your Yarra server\n to a local network drive (usually, using 'net use ...').");
    ui->ortConnectLabel->setToolTip(ui->ortConnectEdit->toolTip());

    ui->ortDisconnectEdit->setToolTip("Enter the command-shell command for disconnecting the network connection to the Yarra server. \nLeave the field empty if you want to keep the Yarra share connected.");
    ui->ortDisconnectLabel->setToolTip(ui->ortConnectEdit->toolTip());

    ui->ortServerPathEdit->setToolTip("Enter the path to the queuing directory of the Yarra server, including \nthe network drive letter for mapped network shares.");
    ui->ortServerPathLabel->setToolTip(ui->ortConnectEdit->toolTip());

    ui->ortFallbackEdit->setToolTip("Enter the command-shell command for mapping the queueing share from a fallback YarraServer.");
    ui->ortFallbackConnectCmdLabel->setToolTip(ui->ortConnectEdit->toolTip());

}


rdsConfigurationWindow::~rdsConfigurationWindow()
{
    delete ui;
}


void rdsConfigurationWindow::closeEvent(QCloseEvent *event)
{
    RTI->setMode(rdsRuntimeInformation::RDS_OPERATION);
}


bool rdsConfigurationWindow::checkAccessPassword()
{
    // Password check to prevent unauthorized users to change
    // the settings
    QInputDialog pwdDialog;
    pwdDialog.setInputMode(QInputDialog::TextInput);
    pwdDialog.setWindowIcon(RDS_ICON);
    pwdDialog.setWindowTitle("Password");
    pwdDialog.setLabelText("Configuration Password:");
    pwdDialog.setTextEchoMode(QLineEdit::Password);

    Qt::WindowFlags flags = pwdDialog.windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    pwdDialog.setWindowFlags(flags);

    if (pwdDialog.exec()==QDialog::Rejected)
    {
        return false;
    }
    else
    {
        return (pwdDialog.textValue()==RDS_PASSWORD);
    }
}


void rdsConfigurationWindow::saveSettings()
{
    storeConfiguration();
    close();
}


void rdsConfigurationWindow::callUpdateModeChanged(int index)
{
    int stackPage=0;

    switch (index)
    {
    case rdsConfiguration::UPDATEMODE_STARTUP:
    case rdsConfiguration::UPDATEMODE_MANUAL:
    default:
        stackPage=0;
        break;

    case rdsConfiguration::UPDATEMODE_FIXEDTIME:
    case rdsConfiguration::UPDATEMODE_STARTUP_FIXEDTIME:
        stackPage=1;
        break;

    case rdsConfiguration::UPDATEMODE_PERIODIC:
        stackPage=2;
        break;
    }

    ui->updateStackedWidget->setCurrentIndex(stackPage);
}


void rdsConfigurationWindow::readConfiguration()
{
    ui->softwareVersionEdit->setText(RTI->getSyngoMRVersionString());

    config.loadConfiguration();

    // Transfer configuration to UI controls
    ui->systemNameEdit->setText(config.infoName);
    ui->time1Edit->setTime(config.infoUpdateTime1);
    ui->time2Edit->setTime(config.infoUpdateTime2);
    ui->time3Edit->setTime(config.infoUpdateTime3);

    ui->time2Checkbox->setChecked(config.infoUpdateUseTime2);
    ui->time3Checkbox->setChecked(config.infoUpdateUseTime3);

    //ui->networkModeCombobox->setCurrentIndex(config.netMode);
    //NOTE: Currently, the software only supports the network drive mode
    ui->networkModeCombobox->setCurrentIndex(0);

    ui->networkDrivePathEdit->setText(config.netDriveBasepath);
    ui->networkDriveReconnectCmd->setText(config.netDriveReconnectCmd);
    ui->networkDriveCreatePath->setChecked(config.netDriveCreateBasepath);

    ui->ftpIPEdit->setText(config.netFTPIP);
    ui->ftpUserEdit->setText(config.netFTPUser);
    ui->ftpPwdEdit->setText(config.netFTPPassword);
    ui->ftpPathEdit->setText(config.netFTPBasepath);

    ui->updateCombobox->setCurrentIndex(config.infoUpdateMode);
    ui->updatePeriodCombobox->setCurrentIndex(config.infoUpdatePeriodUnit);
    ui->updatePeriodSpinbox->setValue(config.infoUpdatePeriod);

    updateProtocolList();

    if (config.getProtocolCount()>0)
    {
        ui->protListWidget->setCurrentRow(0);
    }
    callShowProt();

    ui->ortServerPathEdit->setText(config.ortServerPath);
    ui->ortConnectEdit->setText(config.ortConnectCmd);
    ui->ortDisconnectEdit->setText(config.ortDisconnectCmd);
    ui->ortFallbackEdit->setText(config.ortFallbackConnectCmd);
}


void rdsConfigurationWindow::storeConfiguration()
{
    config.infoName=ui->systemNameEdit->text();

    config.infoUpdateTime1=ui->time1Edit->time();
    config.infoUpdateTime2=ui->time2Edit->time();
    config.infoUpdateTime3=ui->time3Edit->time();

    config.infoUpdateUseTime2=ui->time2Checkbox->isChecked();
    config.infoUpdateUseTime3=ui->time3Checkbox->isChecked();

    config.netMode=ui->networkModeCombobox->currentIndex();
    config.netDriveBasepath=ui->networkDrivePathEdit->text();
    config.netDriveReconnectCmd=ui->networkDriveReconnectCmd->text();
    config.netDriveCreateBasepath=ui->networkDriveCreatePath->isChecked();

    config.netFTPIP=ui->ftpIPEdit->text();
    config.netFTPUser=ui->ftpUserEdit->text();
    config.netFTPPassword=ui->ftpPwdEdit->text();  // TODO: Scramble passoword!!!
    config.netFTPBasepath=ui->ftpPathEdit->text();    

    config.infoUpdateMode=ui->updateCombobox->currentIndex();
    config.infoUpdatePeriodUnit=ui->updatePeriodCombobox->currentIndex();
    config.infoUpdatePeriod=ui->updatePeriodSpinbox->value();

    config.ortServerPath=ui->ortServerPathEdit->text();
    config.ortConnectCmd=ui->ortConnectEdit->text();
    config.ortDisconnectCmd=ui->ortDisconnectEdit->text();
    config.ortFallbackConnectCmd=ui->ortFallbackEdit->text();

    // Write the configuration to the ini file
    config.saveConfiguration();
}


void rdsConfigurationWindow::updateProtocolList()
{
    ui->protListWidget->clear();

    for (int i=0; i<config.getProtocolCount(); i++)
    {
        QString name="";
        QString filter="";
        bool saveAdjustData=false;
        bool anonymizeData=false;
        bool smallFiles=false;
        bool remotelyDefined=false;

        config.readProtocol(i, name, filter, saveAdjustData, anonymizeData, smallFiles, remotelyDefined);
        ui->protListWidget->addItem(name);
    }

    bool uiControlsEnabled=true;
    if (config.getProtocolCount()==0)
    {
        uiControlsEnabled=false;
    }

    ui->protListWidget->setEnabled(uiControlsEnabled);
    ui->protNameEdit->setEnabled(uiControlsEnabled);
    ui->protFilterEdit->setEnabled(uiControlsEnabled);
    ui->protAdjustCheckbox->setEnabled(uiControlsEnabled);
    ui->protAnonymizeCheckbox->setEnabled(uiControlsEnabled);
    ui->protNameLabel->setEnabled(uiControlsEnabled);
    ui->protFilterLabel->setEnabled(uiControlsEnabled);
    ui->protSmallFilesCheckbox->setEnabled(uiControlsEnabled);
}


void rdsConfigurationWindow::callAddProt()
{
    config.addProtocol("Untitled", "_RDS", false, false, false, false);
    updateProtocolList();
    ui->protListWidget->setCurrentRow(ui->protListWidget->count()-1);
}


void rdsConfigurationWindow::callRemoveProt()
{
    int itemToDelete=ui->protListWidget->currentRow();

    if (itemToDelete!=-1)
    {
        config.deleteProtocol(itemToDelete);
        updateProtocolList();
    }
}


void rdsConfigurationWindow::callUpdateProt()
{
    int index=ui->protListWidget->currentRow();

    if ((index!=-1) && (index<config.getProtocolCount()))
    {
        config.updateProtocol(index, ui->protNameEdit->text(), ui->protFilterEdit->text(), ui->protAdjustCheckbox->isChecked(),
                              ui->protAnonymizeCheckbox->isChecked(), ui->protSmallFilesCheckbox->isChecked(), false);
        ui->protListWidget->item(index)->setText(ui->protNameEdit->text());
    }

    //updateProtocolList();
}


void rdsConfigurationWindow::callShowProt()
{
    int index=ui->protListWidget->currentRow();

    if ((index!=-1) && (index<config.getProtocolCount()))
    {
        QString name="";
        QString filter="";
        bool saveAdjustData=false;
        bool anonymizeData=false;
        bool smallFiles=false;
        bool remotelyDefined=false;

        config.readProtocol(index, name, filter, saveAdjustData, anonymizeData, smallFiles, remotelyDefined);

        ui->protNameEdit->setText(name);
        ui->protFilterEdit->setText(filter);
        ui->protAdjustCheckbox->setChecked(saveAdjustData);
        ui->protAnonymizeCheckbox->setChecked(anonymizeData);
        ui->protSmallFilesCheckbox->setChecked(smallFiles);
    }
}


void rdsConfigurationWindow::on_protListWidget_currentRowChanged(int currentRow)
{
    callShowProt();
}


void rdsConfigurationWindow::on_protNameEdit_textEdited(const QString &arg1)
{
    callUpdateProt();
}


void rdsConfigurationWindow::on_protFilterEdit_textEdited(const QString &arg1)
{
    callUpdateProt();
}


void rdsConfigurationWindow::on_protAdjustCheckbox_toggled(bool checked)
{
    callUpdateProt();
}


void rdsConfigurationWindow::on_protAnonymizeCheckbox_toggled(bool checked)
{
    callUpdateProt();
}


void rdsConfigurationWindow::on_networkModeCombobox_currentIndexChanged(int index)
{
  ui->networkStackedWidget->setCurrentIndex(index);
}


void rdsConfigurationWindow::on_networkFilePathButton_clicked()
{
    QString newPath=ui->networkDrivePathEdit->text();
    newPath=QFileDialog::getExistingDirectory(this, "Select Base Path", newPath);
    if (newPath!="")
    {
        ui->networkDrivePathEdit->setText(newPath);

        QMessageBox msgBox;
        msgBox.setWindowIcon(RDS_ICON);
        msgBox.setWindowTitle("Network Drive Path");
        msgBox.setText("<b>NOTE:</b> Ensure that the network drive will be <b>reconnected automatically</b> after restart of the system. "\
                       "Otherwise transfering the raw data will not be possible. If this cannot be ensured, use FTP server connection instead.");
        msgBox.exec();
    }
}


void rdsConfigurationWindow::on_protSmallFilesCheckbox_toggled(bool checked)
{
    callUpdateProt();
}
