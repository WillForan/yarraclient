#include "ort_configurationdialog.h"
#include "ui_ort_configurationdialog.h"

#include <QtWidgets>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QTcpSocket>

#include "ort_global.h"
#include "../Client/rds_global.h"
#include <../NetLogger/netlogger.h>


ortConfigurationDialog::ortConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortConfigurationDialog)
{
    ui->setupUi(this);

    setWindowIcon(ORT_ICON);
    setWindowTitle("Yarra - ORT Configuration");

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

    ui->logServerTestLabel->setText("");
}


ortConfigurationDialog::~ortConfigurationDialog()
{
    delete ui;
}


bool ortConfigurationDialog::executeDialog()
{
    ortConfigurationDialog* instance=new ortConfigurationDialog();

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
    ui->autoLaunchRDSCheckbox->setChecked(settings.value("ORT/StartRDSOnShutdown", false).toBool());

    ui->serialNumberEdit->setText(RTI->getConfigInstance()->infoSerialNumber);

    // Read the mail presets for the ORT configuration dialog
    ui->emailPresetsEdit->clear();
    int mCount=1;
    while ((!settings.value("ORT/MailPreset"+QString::number(mCount),"").toString().isEmpty()) && (mCount<100))
    {
        ui->emailPresetsEdit->appendPlainText(settings.value("ORT/MailPreset"+QString::number(mCount),"").toString());
        mCount++;
    }

    ui->logServerAddressEdit->setText(settings.value("LogServer/ServerAddress", "").toString());
    ui->logServerAPIKeyEdit->setText(settings.value("LogServer/APIKey", "").toString());
}


void ortConfigurationDialog::writeSettings()
{
    QSettings settings(RTI->getAppPath()+"/"+ORT_INI_NAME, QSettings::IniFormat);

    settings.setValue("ORT/SystemName", ui->systemNameEdit->text());
    settings.setValue("ORT/ServerPath", ui->serverPathEdit->text());
    settings.setValue("ORT/ConnectCmd", ui->connectCmdEdit->text());
    settings.setValue("ORT/DisconnectCmd", ui->disconnectCmdEdit->text());
    settings.setValue("ORT/FallbackConnectCmd", ui->fallbackConnectCmdEdit->text());
    settings.setValue("ORT/StartRDSOnShutdown", ui->autoLaunchRDSCheckbox->isChecked());

    QStringList emailList=ui->emailPresetsEdit->toPlainText().split("\n",QString::SkipEmptyParts);

    for (int i=0; i<emailList.count(); i++)
    {
        settings.setValue("ORT/MailPreset"+QString::number(i+1), emailList.at(i));
    }
    // Set next entry to "" in ini file to indicate end of list
    settings.setValue("ORT/MailPreset"+QString::number(emailList.count()+1), "");

    settings.setValue("LogServer/ServerAddress",ui->logServerAddressEdit->text());
    settings.setValue("LogServer/APIKey",       ui->logServerAPIKeyEdit->text());
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



void ortConfigurationDialog::on_logServerTestButton_clicked()
{   
    // Reset previous output
    ui->logServerTestLabel->setText("Testing connection...");
    RTI->processEvents();

    const QString errorPrefix="<span style=""color:#990000;""><strong>ERROR:</strong></span>&nbsp;&nbsp;";

    QString output="";
    bool error=false;

    QString serverPath=ui->logServerAddressEdit->text();
    int serverPort=8080;

    if (serverPath.isEmpty())
    {
        output=errorPrefix + "No server address entered.";
        ui->logServerTestLabel->setText(output);

        return;
    }

    // Check if the entered address contains a port specifier
    int colonPos=serverPath.indexOf(":");

    if (colonPos!=-1)
    {
        int cutChars=serverPath.length()-colonPos;

        QString portString=serverPath.right(cutChars-1);
        serverPath.chop(cutChars);

        // Convert port string into number and validate
        bool portValid=false;

        int tempPort=portString.toInt(&portValid);

        if (portValid)
        {
            serverPort=tempPort;
        }
    }

    QString localHostname="";
    QString localIP="";

    // Open socket connection to see if the server is active and to
    // determine the local IP address used for routing to the server.
    QTcpSocket socket;
    socket.connectToHost(serverPath, serverPort);

    if (socket.waitForConnected(10000))
    {
        localIP=socket.localAddress().toString();
    }
    else
    {
        output += errorPrefix + "Unable to connect to server.";
        error=true;
    }

    socket.disconnectFromHost();

    // Lookup the hostname of the local client from the DNS server
    if (!error)
    {
        localHostname=NetLogger::dnsLookup(localIP);

        if (localHostname.isEmpty())
        {
            output += errorPrefix + "Unable to resolve local hostname.<br /><br />IP = " + localIP;
            output += "<br />Check local DNS server settings.";
            error=true;
        }
    }

    // Lookup the hostname of the log server from the DNS server
    if (!error)
    {
        serverPath=NetLogger::dnsLookup(serverPath);

        if (serverPath.isEmpty())
        {
            output += errorPrefix + "Unable to resolve server name.<br /><br />";
            output += "Check local DNS server settings.";
            error=true;
        }
    }

    // Compare if the local system and the server are on the same domain
    if (!error)
    {
        QStringList localHostString =localHostname.toLower().split(".");
        QStringList serverHostString=serverPath.toLower().split(".");

        if ((localHostString.count()<2) || (serverHostString.count()<2))
        {
            output += errorPrefix + "Error resolving hostnames.<br /><br />";
            output += "Local host: " + localHostname.toLower() + " ("+localIP+")<br />";
            output += "Log server: " + serverHostString.join(".");
            error=true;
        }
        else
        {
            if ((localHostString.at(localHostString.count()-1)!=(serverHostString.at(serverHostString.count()-1)))
               || (localHostString.at(localHostString.count()-2)!=(serverHostString.at(serverHostString.count()-2))))
            {
                output += errorPrefix + "Server not in local domain.<br /><br />";
                output += "Local host: " + localHostname.toLower() + "<br />";
                output += "Log server: " + serverPath.toLower() + "<br />";
                error=true;
            }
        }
    }

    // Check if the server responds to the test entry point
    if (!error)
    {
        QUrlQuery data;
        QNetworkReply::NetworkError net_error;
        int http_status=0;

        // Create temporary instance for testing the logserver connection
        NetLogger testLogger;

        // Read the full server path entered into the UI control again. The path returned from the DNS
        // might differ if an DNS alias is used. In this case, the certificate will be incorrect.
        serverPath         =ui->logServerAddressEdit->text();
        QString apiKey     =ui->logServerAPIKeyEdit->text();
        QString name       ="ConnectionTest";
        QString errorString="n/a";

        testLogger.configure(serverPath, EventInfo::SourceType::RDS, name, apiKey, true);
        data.addQueryItem("api_key",apiKey);
        bool success=testLogger.postData(data, NETLOG_ENDPT_TEST, net_error, http_status, errorString);

        if (!success)
        {
            if (http_status==0)
            {
                output += errorPrefix + "No response from server (Error: "+errorString+").<br /><br />";

                switch (net_error)
                {
                case QNetworkReply::NetworkError::SslHandshakeFailedError:
                    output += "Is SSL certificate installed / valid?";
                    break;
                case QNetworkReply::NetworkError::AuthenticationRequiredError:
                    output += "Is API key correct?";
                    break;
                default:
                    output += "Is server running? Is port correct?";
                    break;
                }
            }
            else
            {
                output += errorPrefix + "Server rejected request (Status: "+QString::number(http_status)+").<br /><br />Is API key corrent?";
            }

            error=true;
        }
    }

    if (!error)
    {
        output="<span style=""color:#009900;""><strong>Success.</strong></span>";
    }

    ui->logServerTestLabel->setText(output);
}
