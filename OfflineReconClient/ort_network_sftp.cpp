#include <QtWidgets>

#include "ort_network_sftp.h"
#include "ort_remotefilehelper.h"
#include "ort_configuration.h"
#include "ort_configurationdialog.h"
#include "ort_global.h"


ortNetworkSftp::ortNetworkSftp()
{
}


bool ortNetworkSftp::prepare()
{
    bool result = ortNetwork::prepare();
    if (!result) {
        return false;
    }
    return true;
}


QSettings* ortNetworkSftp::readModelist(QString &error)
{
    helper.get(ORT_MODEFILE, QDir::temp());
    return new QSettings(QDir::temp().absoluteFilePath(ORT_MODEFILE),QSettings::Format::IniFormat);
}


bool ortNetworkSftp::copyFile()
{
    QString sourceName=queueDir.absoluteFilePath(currentFilename);
    return helper.put(sourceName);
}


bool ortNetworkSftp::verifyTransfer()
{
    bool exists = helper.exists(currentFilename);

    if (!exists)
    {
        RTI->log("ERROR: Copied file does not exist at target position: " + currentFilename);
        RTI->log("ERROR: Transferring the file was not successful.");
        RTI->setSevereErrors(true);
        return false;
    }

//  TODO: verify file size
    qlonglong remote_size = helper.size(currentFilename);
    if (remote_size != currentFilesize)
    {
        RTI->log("ERROR: File size of copied file does not match!");
        RTI->log("ERROR: Original size = " + QString::number(currentFilesize));
        RTI->log("ERROR: Copy size = " + QString::number(remote_size));
        RTI->setSevereErrors(true);
        return false;
    }
    RTI->log("File transfer successful.");
    return true;
}


bool ortNetworkSftp::doReconnectServerEntry(ortServerEntry *selectedEntry)
{
    if(serverPath.isEmpty())
    {
        return false;
    }

    helper.init(serverPath, selectedEntry->hostKey);
    QString out;
    return helper.testConnection(out);
}


bool ortNetworkSftp::syncServerList()
{
    bool result = serverList.removeLocalServerList();

    // Check if server list exists in remote directory
    if (!helper.exists(ORT_SERVERLISTFILE))
    {
        if (result)
        {
            // Note in log file is a local version of the server list file has been
            // removed but a new file has not been found on the server. Might indicate
            // a problem with the server.
            RTI->log("Warning: Removed local server list but did not find one on server.");
        }
        return true;
    }

    QString lockFilename=ORT_SERVERLISTFILE;
    lockFilename.truncate(lockFilename.indexOf("."));
    lockFilename+=ORT_LOCK_EXTENSION;

    if (helper.exists(lockFilename))
    {
        {
            int retries=0;
            bool fileUnlocked=false;

            while (retries<4)
            {
                retries++;

                RTI->log("Warning: Server list on remote server is locked. Retrying.");
                Sleep(100);

                if (!helper.exists(lockFilename))
                {
                    fileUnlocked=true;
                }
            }

            if (!fileUnlocked)
            {
                RTI->log("Error: Server list still locked after multiple retries.");
                return false;
            }
        }
    }

    QDir localDir(appPath);
    if (!helper.get(ORT_SERVERLISTFILE,localDir))
    {
        RTI->log("Error: Unable to synchronize server list.");
        return false;
    }
    return true;
}


bool ortNetworkSftp::openConnection(bool fallback)
{
    if (!fallback)
    {
        helper.init(configInstance->ortServerURI);
    }
    else
    {
        helper.init(configInstance->ortFallbackServerURI);
    }

    QString error;
    bool result = helper.testConnection(error);

    if (!result)
    {
        goto on_error;
    }

    if(!helper.exists(QStringList{ORT_MODEFILE, ORT_SERVERFILE}))
    {
        error = "ERROR: ORT mode or server file not found.";
        goto on_error;
    }

    RTI->log("Mode and server files found.");
    RTI->log("Connection validated.");

    if (syncServerList())
    {
        // If copying the server list was successful, read the local
        // copy of the list into memory.
        serverList.readLocalServerList();
    }

    return true;

on_error:
    RTI->log(error);
    RTI->log("Unable to connect to server.");
    QMessageBox msgBox;
    msgBox.setWindowTitle("Error");
    msgBox.setText("Unable to connect:\n" + error + "\n\nDo you want to review the configuration?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setWindowIcon(ORT_ICON);
    msgBox.setIcon(QMessageBox::Critical);

    if (msgBox.exec()==QMessageBox::Yes)
    {
        // Call configuration dialog
        ortConfigurationDialog::executeDialog();
    }
    RTI->setSevereErrors(true);
    return false;
}
