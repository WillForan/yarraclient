#include "ort_serverlist.h"
#include "ort_global.h"

#include <time.h>

ortServerList::ortServerList()
{
    serverListAvailable=false;
    appPath=qApp->applicationDirPath();
}


ortServerList::~ortServerList()
{
    clearList();
}


void ortServerList::clearList()
{
    while (!servers.isEmpty())
    {
        delete servers.takeFirst();
    }
}


bool ortServerList::syncServerList(QString remotePath)
{
    QDir localDir(appPath);

    bool removedServerList=false;

    // Remove the old copy of the serverlist file, if it exists
    if (localDir.exists(ORT_SERVERLISTFILE))
    {
        if (!localDir.remove(ORT_SERVERLISTFILE))
        {
            RTI->log("Error: Cannot remove local chached copy of server list!");
            RTI->log("Error: Check write permission in installation folder.");
            return false;
        }

        removedServerList=true;
    }

    QDir remoteDir(remotePath);

    // Check if server list exists in remote directory
    if (!remoteDir.exists(ORT_SERVERLISTFILE))
    {
        if (removedServerList)
        {
            // Note in log file is a local version of the server list file has been
            // removed but a new file has not been found on the server. Might indicate
            // a problem with the server.
            RTI->log("Warning: Removed local server list but did not find one on server.");
        }

        return true;
    }

    // Check if file is locked at the moment. If so, sleep and retry several times.
    QString lockFilename=ORT_SERVERLISTFILE;
    lockFilename.truncate(lockFilename.indexOf("."));
    lockFilename+=ORT_LOCK_EXTENSION;

    if (remoteDir.exists(lockFilename))
    {
        int retries=0;
        bool fileUnlocked=false;

        while (retries<4)
        {
            retries++;

            RTI->log("Warning: Server list on remote server is locked. Retrying.");
            Sleep(100);

            if (!remoteDir.exists(lockFilename))
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

    if (!QFile::copy(localDir.filePath(ORT_SERVERLISTFILE),remoteDir.filePath(ORT_SERVERLISTFILE)))
    {
        RTI->log("Error: Unable to synchronize server list.");
        return false;
    }

    return true;
}


bool ortServerList::readLocalServerList()
{
    QDir localDir(appPath);

    if (!localDir.exists(ORT_SERVERLISTFILE))
    {
        return false;
    }

    clearList();

    {
        QSettings serverListIni(localDir.filePath(ORT_SERVERLISTFILE), QSettings::IniFormat);
        QStringList serverEntries=serverListIni.childGroups();

        for (int i=0; i<serverEntries.count(); i++)
        {
            QString serverName=serverEntries.at(i);
            serverListIni.beginGroup(serverName);
            if (!serverListIni.value("Disabled",false).toBool())
            {
                ortServerEntry* entry=new ortServerEntry();
                entry->name      =serverName;
                entry->type      =serverListIni.value("Type","").toString().toLower();
                entry->connectCmd=serverListIni.value("ConnectCmd","").toString();

                // If a Base64 encoded version is availabe, decode it and overwrite
                // the other setting.
                QString connectCmdE=serverListIni.value("ConnectCmdE","").toString();
                if (connectCmdE!="")
                {
                    entry->connectCmd=QString(QByteArray::fromBase64(connectCmdE.toLatin1()));
                }

                servers.append(entry);
            }
            // Check if enabled
            serverListIni.endGroup();
        }
    }

    return true;
}


int ortServerList::findMatchingServers(QString type)
{
    // Convert the given type to lower case, so that the comparison
    // is more reliable
    type=type.toLower();

    matchingServers.clear();

    if (type.isEmpty())
    {
        matchingServers=servers;
    }
    else
    {
        for (int i=0; i<servers.count(); i++)
        {
            if (servers.at(i)->type==type)
            {
                matchingServers.append(servers.at(i));
            }
        }
    }

    return matchingServers.count();
}


ortServerEntry* ortServerList::getNextMatchingServer()
{
    if (matchingServers.isEmpty())
    {
        return 0;
    }

    // Poor man's load balancing for now...
    srand(time(NULL));
    int selectedIndex=rand() % matchingServers.count();

    ortServerEntry* selectedEntry=matchingServers.at(selectedIndex);
    matchingServers.removeAt(selectedIndex);

    return selectedEntry;
}
