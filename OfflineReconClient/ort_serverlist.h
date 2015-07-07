#ifndef ORT_SERVERLIST_H
#define ORT_SERVERLIST_H

#include <QtCore>

class ortServerEntry
{
public:
    QString     name;
    QStringList type;
    QString     connectCmd;
    bool        acceptsUndefined;
};


class ortServerList
{
public:
    ortServerList();
    ~ortServerList();

    QList<ortServerEntry*> servers;
    QList<ortServerEntry*> matchingServers;

    void clearList();
    bool syncServerList(QString remotePath);
    bool readLocalServerList();
    bool isServerListAvailable();
    ortServerEntry* getServerEntry(QString name);

    int findMatchingServers(QString type);
    ortServerEntry* getNextMatchingServer();

protected:

    bool    serverListAvailable;
    QString appPath;

};


inline bool ortServerList::isServerListAvailable()
{
    return serverListAvailable;
}



#endif // ORT_SERVERLIST_H
