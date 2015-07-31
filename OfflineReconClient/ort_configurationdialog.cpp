#include "ort_configurationdialog.h"
#include "ui_ort_configurationdialog.h"

#include <QtWidgets>
#include "ort_global.h"


ortConfigurationDialog::ortConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortConfigurationDialog)
{
    ui->setupUi(this);
    requiresRestart=false;
}


ortConfigurationDialog::~ortConfigurationDialog()
{
    delete ui;
}


bool ortConfigurationDialog::executeDialog()
{
    ortConfigurationDialog* instance=new ortConfigurationDialog;
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
