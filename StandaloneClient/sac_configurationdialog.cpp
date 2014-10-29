#include <QtWidgets>

#include "sac_configurationdialog.h"
#include "sac_mainwindow.h"

#include "ui_sac_configurationdialog.h"

sacConfigurationDialog::sacConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sacConfigurationDialog)
{
    ui->setupUi(this);

    setWindowTitle("Configuration");

    mainWindow=0;
    closeMainWindow=false;
}


sacConfigurationDialog::~sacConfigurationDialog()
{
    delete ui;
}


void sacConfigurationDialog::prepare(sacMainWindow* mainWindowPtr)
{
    mainWindow=mainWindowPtr;

    ui->connectEdit->setText(mainWindow->network.connectCmd);
    ui->disconnectEdit->setText(mainWindow->network.disconnectCmd);
    ui->serverpathEdit->setText(mainWindow->network.serverPath);
    ui->nameEdit->setText(mainWindow->network.systemName);
    ui->notificationEdit->setText(mainWindow->network.defaultNotification);

    for (int i=0; i<mainWindow->modeList.modes.count(); i++)
    {
        ui->modeCombobox->addItem(mainWindow->modeList.modes.at(i)->readableName);
    }
    ui->modeCombobox->setCurrentIndex(mainWindow->defaultMode);
}


void sacConfigurationDialog::on_cancelButton_clicked()
{
    close();
}


void sacConfigurationDialog::on_saveButton_clicked()
{
    mainWindow->network.connectCmd=ui->connectEdit->text();
    mainWindow->network.disconnectCmd=ui->disconnectEdit->text();
    mainWindow->network.serverPath=ui->serverpathEdit->text();
    mainWindow->network.systemName=ui->nameEdit->text();
    mainWindow->network.defaultNotification=ui->notificationEdit->text();

    int selectMode=ui->modeCombobox->currentIndex();
    if ((selectMode==-1) || (selectMode>=mainWindow->modeList.modes.count()))
    {
        mainWindow->network.preferredMode="";
    }
    else
    {
        mainWindow->network.preferredMode=mainWindow->modeList.modes.at(selectMode)->idName;
    }

    mainWindow->network.writeConfiguration();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Configuration Saved");
    msgBox.setText("The configuration has been saved.\n\nThe SAC client will be restarted to activate the settings.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();

    closeMainWindow=true;
    close();
}

