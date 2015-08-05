#include "ort_serverlist.h"
#include "ort_global.h"

#include <time.h>


ortServerList::ortServerList()
{
    serverListAvailable=false;
    appPath=qApp->applicationDirPath();
    selectedIndex=0;
    servers.clear();
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
        // Reset the file permissions of the local file, in case it was copied
        // from the network with read-only permissions
        {
            QFile existingFile(localDir.absoluteFilePath(ORT_SERVERLISTFILE));
            existingFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther);
        }

        if (!localDir.remove(ORT_SERVERLISTFILE))
        {
            RTI->log("Error: Cannot remove local chached copy of server list!");
            RTI->log("Error: Check write permission in installation folder.");

            removedServerList=false;
            return false;
        }
        else
        {
            removedServerList=true;
        }
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

    if (!QFile::copy(remoteDir.filePath(ORT_SERVERLISTFILE),localDir.filePath(ORT_SERVERLISTFILE)))
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
                entry->type      =serverListIni.value("Type","").toStringList();
                entry->connectCmd=serverListIni.value("ConnectCmd","").toString();

                // If a Base64 encoded version is availabe, decode it and overwrite
                // the other setting.
                QString connectCmdE=serverListIni.value("ConnectCmdE","").toString();
                if (connectCmdE!="")
                {
                    entry->connectCmd=QString(QByteArray::fromBase64(connectCmdE.toLatin1()));
                }

                entry->acceptsUndefined=serverListIni.value("AcceptsUndefined",true).toBool();

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
    matchingServers.clear();
    selectedIndex=0;

    if (type.isEmpty())
    {
        // If no server type has been requested, consider all
        // servers that accept tasks with undefined type
        for (int i=0; i<servers.count(); i++)
        {
            if (servers.at(i)->acceptsUndefined)
            {
                matchingServers.append(servers.at(i));
            }
        }
    }
    else
    {
        for (int i=0; i<servers.count(); i++)
        {
            if (servers.at(i)->type.contains(type, Qt::CaseInsensitive))
            {
                matchingServers.append(servers.at(i));
            }
        }
    }

    // Read the starting index (i.e. the index of the first server that should
    // be tried). This index is increased each time to achieve load balancing.
    getLoadBalancingIndex(type);

    return matchingServers.count();
}


ortServerEntry* ortServerList::getNextMatchingServer()
{
    if (matchingServers.isEmpty())
    {
        return 0;
    }

    // Poor man's load balancing for now...
    //srand(time(NULL));
    //int selectedIndex=rand() % matchingServers.count();

    if (selectedIndex>=matchingServers.count())
    {
        selectedIndex=0;
    }

    ortServerEntry* selectedEntry=matchingServers.at(selectedIndex);
    matchingServers.removeAt(selectedIndex);

    return selectedEntry;
}


ortServerEntry* ortServerList::getServerEntry(QString name)
{
    for (int i=0; i<servers.count(); i++)
    {
        if (servers.at(i)->name==name)
        {
            return servers.at(i);
        }
    }

    return 0;
}


void ortServerList::getLoadBalancingIndex(QString type)
{
    // Use "Undefined" for non-specified type (to comply ini format)
    if (type.isEmpty())
    {
        type="Undefined";
    }

    // Read the index value from a local ini file
    QSettings lbiFile(RTI->getAppPath()+"/"+ORT_LBI_NAME, QSettings::IniFormat);
    selectedIndex=lbiFile.value("LoadBalancingIndex/"+type, 0).toInt();

    // The modulo operation is used because fewer servers could exist now.
    if (matchingServers.count()!=0)
    {
        selectedIndex=selectedIndex % matchingServers.count();
    }
    else
    {
        selectedIndex=0;
    }

    // Increase and save value again
    lbiFile.setValue("LoadBalancingIndex/"+type, selectedIndex+1);
}




