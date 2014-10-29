#ifndef ORT_MODELIST_H
#define ORT_MODELIST_H

#include <QtCore>


class ortModeEntry
{
public:
    QString idName;
    QString readableName;
    QString protocolTag;
    QString mailConfirmation;
    bool requiresACC;
    bool requiresAdjScans;
    double minimumSizeMB;

    QString paramLabel;
    QString paramDescription;
    int     paramDefault;
    int     paramMin;
    int     paramMax;
};


#ifdef YARRA_APP_ORT
class ortNetwork;
#endif
#ifdef YARRA_APP_SAC
class sacNetwork;
#endif

class ortModeList
{
public:
    ortModeList();
    ~ortModeList();

    bool readModeList();
    int getModeForProtocol(QString protocol);

    QList<ortModeEntry*> modes;
    QString serverName;
    QString serverType;
    int count;    

#ifdef YARRA_APP_ORT
    ortNetwork* network;
#endif
#ifdef YARRA_APP_SAC
    sacNetwork* network;
#endif

};



inline int ortModeList::getModeForProtocol(QString protocol)
{
    for (int i=0; i<modes.count(); i++)
    {
        if (protocol.contains(modes.at(i)->protocolTag))
        {
            return i;
        }
    }

    return -1;
}


#endif // ORT_MODELIST_H

