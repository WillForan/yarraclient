#ifndef ORT_MODELIST_H
#define ORT_MODELIST_H

#include <QtCore>


class ortModeEntry
{
public:

    enum ComputeMode
    {
        OnPremise=0,
        Cloud,
        Elastic
    };

    ortModeEntry();

    QString     idName;
    QString     readableName;
    QString     protocolTag;
    QString     mailConfirmation;
    bool        requiresACC;
    bool        requiresAdjScans;
    double      minimumSizeMB;
    QString     requiredServerType;
    ComputeMode computeMode; 
    bool        requestAdditionalFiles;

    // User selectable parameter 1
    QString     paramLabel;
    QString     paramDescription;
    double      paramDefault;
    double      paramMin;
    double      paramMax;
    bool        paramIsFloat;

    // User selectable parameter 2
    QString     param2Label;
    QString     param2Description;
    double      param2Default;
    double      param2Min;
    double      param2Max;
    bool        param2IsFloat;
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

