#include <QtWidgets>

#include "sac_configurationdialog.h"
#include "sac_mainwindow.h"
#include "sac_global.h"

#include "ui_sac_configurationdialog.h"

#include "../CloudTools/yct_common.h"
#include "../CloudTools/yct_aws/qtaws.h"


sacConfigurationDialog::sacConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sacConfigurationDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    setWindowTitle("Yarra - SAC Configuration");
    ui->versionLabel->setText("Version " + QString(SAC_VERSION) + ", Build date " + QString(__DATE__));

    mainWindow=0;
    closeMainWindow=false;

    // Center the window on the screen
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,size(),
                                    qApp->desktop()->availableGeometry()));

    ui->tabWidget->setCurrentIndex(0);
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

    ui->cloudCheckbox->setChecked(mainWindow->network.cloudSupportEnabled);
    this->on_cloudCheckbox_clicked(mainWindow->network.cloudSupportEnabled);
    this->updateCloudCredentialStatus();
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
    mainWindow->network.cloudSupportEnabled=ui->cloudCheckbox->isChecked();

    mainWindow->network.writeConfiguration();
    mainWindow->cloudConfig.saveConfiguration();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Configuration Saved");
    msgBox.setText("The configuration has been saved.\n\nThe SAC client will be restarted to activate the settings.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();

    closeMainWindow=true;
    close();
}


void sacConfigurationDialog::on_cloudCheckbox_clicked(bool checked)
{
    ui->cloudConnectionButton ->setEnabled(checked);
    ui->cloudCredetialsEdit   ->setEnabled(checked);
    ui->cloudCredentialsButton->setEnabled(checked);
    ui->cloudCredetialsLabel  ->setEnabled(checked);
}


void sacConfigurationDialog::on_cloudCredentialsButton_clicked()
{
    QString awsKey=QInputDialog::getText(this, "Enter the YarraCloud Key ID", "Please enter your YarraCloud Key ID.<br>&nbsp;<br>If you don't have this information, sign into http://admin.yarracloud.com and go to Configuration -> Access Keys.");
    if (awsKey.isEmpty())
    {
        return;
    }

    QString awsSecret=QInputDialog::getText(this, "Enter the YarraCloud Secret Key", "Please enter your YarraCloud Secret Key.<br>&nbsp;<br>If you don't have this information, sign into http://admin.yarracloud.com and go to Configuration -> Access Keys.");
    if (awsSecret.isEmpty())
    {
        return;
    }

    mainWindow->cloudConfig.key=awsKey;
    mainWindow->cloudConfig.secret=awsSecret;

    updateCloudCredentialStatus();
}


void sacConfigurationDialog::updateCloudCredentialStatus()
{
    if (mainWindow->cloudConfig.isConfigurationValid())
    {
        ui->cloudCredetialsEdit->setText(mainWindow->cloudConfig.key);
    }
    else
    {
        ui->cloudCredetialsEdit->setText("-- Missing --");
    }

}


void sacConfigurationDialog::on_cloudConnectionButton_clicked()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QtAWSRequest awsRequest(mainWindow->cloudConfig.key, mainWindow->cloudConfig.secret);
    QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/user_status",
                                            QByteArray(), YCT_API_REGION, QByteArray(), QStringList());

    bool success=true;
    QString errorReason="";
    QString userRegion="";

    qDebug() << reply.replyData();

    if (!reply.isSuccess())
    {
        qDebug() << reply.anyErrorString();
        errorReason=reply.anyErrorString();

        //errorReason="Network error: " + QString::number((int) reply.networkError());

        if (reply.networkError()==QNetworkReply::ContentOperationNotPermittedError)
        {
            errorReason="YarraCloud Key ID or Secret is invalid.";
        }

        success=false;
    }

    if (success)
    {
        QJsonDocument jsonReply =QJsonDocument::fromJson(reply.replyData());
        QJsonObject   jsonObject=jsonReply.object();
        QStringList   keys      =jsonObject.keys();

        foreach(QString key, keys)
        {
            qDebug() << key << ": " << jsonObject[key];

            if (key=="can_submit_jobs")
            {
                success=jsonObject[key].toBool();
                errorReason="Missing payment method or account has been disabled.";
            }

            if (key=="region")
            {
                userRegion=jsonObject[key].toString();
            }
        }
    }


    if (!success)
    {
        ui->cloudConnectionLabel->setText("<span style=""color:#990000;""><strong>&nbsp; Failure</strong></span>");
    }
    else
    {
        ui->cloudConnectionLabel->setText("<span style=""color:#009900;""><strong>&nbsp; Success</strong></span>&nbsp; (Region: " + userRegion + ")");
    }

    QApplication::restoreOverrideCursor();
    RTI->processEvents();

    if (!success)
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("YarraCloud Problem");
        msgBox.setText("Testing the connection to YarraCloud was <strong>not successful</strong>.<br><br>Reason: "+errorReason);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
}
