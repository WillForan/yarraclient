#include "ort_configurationdialog.h"
#include "ui_ort_configurationdialog.h"

#include <QtWidgets>
#include "ort_global.h"
#include "../Client/rds_global.h"


ortConfigurationDialog::ortConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortConfigurationDialog)
{
    ui->setupUi(this);

    setWindowIcon(ORT_ICON);
    setWindowTitle("Yarra ORT Configuration");

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    ui->versionLabel->setText("Version " + QString(ORT_VERSION) + ", Build date " + QString(__DATE__));

    ui->systemNameEdit->setToolTip("Please enter a unique name to identify this MR system.");
    ui->systemNameLabel->setToolTip(ui->systemNameEdit->toolTip());

    ui->serverPathEdit->setToolTip("Enter the path to the queuing directory of the Yarra server, including \nthe network drive letter for mapped network shares.");
    ui->serverPathLabel->setToolTip(ui->serverPathEdit->toolTip());

    ui->connectCmdEdit->setToolTip("Enter the full command-shell command for mapping the share from your Yarra server\n to a local network drive (usually, using 'net use ...').");
    ui->connectCmdLabel->setToolTip(ui->connectCmdEdit->toolTip());

    ui->disconnectCmdEdit->setToolTip("Enter the command-shell command for disconnecting the network connection to the Yarra server. \nLeave the field empty if you want to keep the Yarra share connected.");
    ui->disconnectCmdLabel->setToolTip(ui->disconnectCmdEdit->toolTip());

    ui->fallbackConnectCmdEdit->setToolTip("Enter the command-shell command for mapping the queueing share from a fallback YarraServer.");
    ui->fallbackConnectCmdLabel->setToolTip(ui->fallbackConnectCmdEdit->toolTip());

    ui->softwareVersionEdit->setToolTip("Detected software version of the Siemens SyngoMR software.");
    ui->softwareVersionLabel->setToolTip(ui->softwareVersionEdit->toolTip());
    ui->softwareVersionEdit->setText(RTI->getSyngoMRVersionString());

    requiresRestart=false;
    ui->tabWidget->setCurrentIndex(0);

    // Center the window on the screen
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,size(),
                                    qApp->desktop()->availableGeometry()));

    readSettings();
}


ortConfigurationDialog::~ortConfigurationDialog()
{
    delete ui;
}


bool ortConfigurationDialog::executeDialog()
{
    ortConfigurationDialog* instance=new ortConfigurationDialog;

    if (!instance->checkAccessPassword())
    {
        return false;
    }

    instance->exec();
    bool restartNeeded=instance->requiresRestart;

    delete instance;
    instance=0;

    if (restartNeeded)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Restart Needed");
        msgBox.setText("Please restart the Yarra client for activating the modified configuration.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(ORT_ICON);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }

    return restartNeeded;
}


void ortConfigurationDialog::on_okButton_clicked()
{
    if (ui->systemNameEdit->text().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("System Name Needed");
        msgBox.setText("Please enter a unique name for the MR system.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(ORT_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        ui->systemNameEdit->setFocus();
        return;
    }

    if (ui->serverPathEdit->text().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Server Path Needed");
        msgBox.setText("Please enter the network path of the Yarra server.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(ORT_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        ui->serverPathEdit->setFocus();
        return;
    }

    // Save configuration
    writeSettings();

    requiresRestart=true;
    close();
}


void ortConfigurationDialog::on_cancelButton_clicked()
{
    requiresRestart=false;
    close();
}


void ortConfigurationDialog::readSettings()
{
    QSettings settings(RTI->getAppPath()+"/"+ORT_INI_NAME, QSettings::IniFormat);

    ui->systemNameEdit->setText(settings.value("ORT/SystemName", "").toString());
    ui->serverPathEdit->setText(settings.value("ORT/ServerPath", "").toString());
    ui->connectCmdEdit->setText(settings.value("ORT/ConnectCmd", "").toString());
    ui->disconnectCmdEdit->setText(settings.value("ORT/DisconnectCmd", "").toString());
    ui->fallbackConnectCmdEdit->setText(settings.value("ORT/FallbackConnectCmd", "").toString());

    // Read the mail presets for the ORT configuration dialog
    ui->emailPresetsEdit->clear();
    int mCount=1;
    while ((!settings.value("ORT/MailPreset"+QString::number(mCount),"").toString().isEmpty()) && (mCount<100))
    {
        ui->emailPresetsEdit->appendPlainText(settings.value("ORT/MailPreset"+QString::number(mCount),"").toString());
        mCount++;
    }
}


void ortConfigurationDialog::writeSettings()
{
    QSettings settings(RTI->getAppPath()+"/"+ORT_INI_NAME, QSettings::IniFormat);

    settings.setValue("ORT/SystemName", ui->systemNameEdit->text());
    settings.setValue("ORT/ServerPath", ui->serverPathEdit->text());
    settings.setValue("ORT/ConnectCmd", ui->connectCmdEdit->text());
    settings.setValue("ORT/DisconnectCmd", ui->disconnectCmdEdit->text());
    settings.setValue("ORT/FallbackConnectCmd", ui->fallbackConnectCmdEdit->text());

    QStringList emailList=ui->emailPresetsEdit->toPlainText().split("\n",QString::SkipEmptyParts);

    for (int i=0; i<emailList.count(); i++)
    {
        settings.setValue("ORT/MailPreset"+QString::number(i+1), emailList.at(i));
    }
    // Set next entry to "" in ini file to indicate end of list
    settings.setValue("ORT/MailPreset"+QString::number(emailList.count()+1), "");
}


bool ortConfigurationDialog::checkAccessPassword()
{
    // Password check to prevent unauthorized users to change
    // the settings
    QInputDialog pwdDialog;
    pwdDialog.setInputMode(QInputDialog::TextInput);
    pwdDialog.setWindowIcon(ORT_ICON);
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


