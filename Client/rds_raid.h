#ifndef RDS_RAID_H
#define RDS_RAID_H

#include <QtWidgets>

#include "rds_global.h"


class rdsRaidEntry
{
public:
    int       fileID;
    int       measID;
    QString   protName;
    QString   patName;
    qint64    size;
    qint64    sizeOnDisk;
    QDateTime creationTime;
    QDateTime closingTime;

    void addToUrlQuery(QUrlQuery& query);
};


class rdsExportEntry
{
public:
    int     raidIndex;
    int     protIndex;
    QString protName;
    bool    anonymize;
    bool    adjustmentScans;
};


class rdsRaid : public QObject
{
    Q_OBJECT

public:
    rdsRaid();
    ~rdsRaid();

    bool createExportList(bool onlyReadRaid=false);
    bool processTotalExportList();
    bool processExportListEntry();
    bool exportsAvailable();

    void dumpRaidList();
    void dumpRaidToolOutput();

    void chopDependingIDs(QString& text);
    bool isPatchedRaidToolMissing();

    bool readRaidList();
    bool saveRaidFile(int fileID, QString filename, bool saveAdjustments=false, bool anonymize=false);

    bool findAdjustmentScans();

    QList<rdsRaidEntry*> raidList;
    QList<int> currentAdjustScans;

    int     currentRaidIndex;
    int     currentFileID;
    QString currentFilename;

    void setIgnoreLPFI();

    // For ORT-use only
    bool saveSingleFile(int fileID,  bool saveAdjustments, QString modeID,
                        QString &filename, QStringList& adjustFilenames, QString paramSuffix);

    bool ortMissingDiskspace;
    QString ortSystemName;

    void setORTSystemName(QString name);

    bool isScanActive();

protected:

    bool parseVB15Line(QString line, rdsRaidEntry* entry);

    bool parseOutputDirectory();

    bool exportScanFromList();

    bool setCurrentFileID();
    bool setCurrentFilename(int refID=-1);

    bool parseOutputFileExport();

    bool anonymizeCurrentFile();

    int  getLPFI();
    void setLPFI(int value);
    void readLPFI();
    void saveLPFI();

    bool callRaidTool(QStringList command, QStringList options);

    QStringList raidToolOutput;

    QDir queueDir;
    QString raidToolCmd;
    QString raidToolIP;

    QList<rdsExportEntry*> exportList;

    int lastProcessedFileID;

    void clearRaidList();
    void clearExportList();

    rdsExportEntry* getFirstExportEntry();
    rdsRaidEntry*   getFirstRaidEntry();
    rdsRaidEntry*   getRaidEntry(int index);

    void addExportEntry(int raidIndex, int protIndex, QString protName, bool anonymize, bool adjustmentScans);
    void addRaidEntry(int fileID, int measID, QString protName, QString patName, qint64 size, qint64 sizeOnDisk, QDateTime creationTime);
    void addRaidEntry(rdsRaidEntry* source);

    void removePrecedingSpace(QString& text);

    bool usePatchedRaidTool;
    bool patchedRaidToolMissing;

    bool ignoreLPFID;
    bool scanActive;

    QString getORTFilename(rdsRaidEntry* entry, QString modeID, QString param, int refID=-1);

};


inline int rdsRaid::getLPFI()
{
    return lastProcessedFileID;
}


inline void rdsRaid::setLPFI(int value)
{
    RTI->debug("Setting LPFI to " + QString::number(value));
    lastProcessedFileID=value;
}


inline void rdsRaid::clearRaidList()
{
    // Dispose all entries in the raid list
    while (!raidList.isEmpty())
    {
        delete raidList.takeFirst();
    }
}


inline void rdsRaid::clearExportList()
{
    // Dispose all entries in the export work list
    while (!exportList.isEmpty())
    {
        delete exportList.takeFirst();
    }
}


inline rdsRaidEntry* rdsRaid::getRaidEntry(int index)
{
    if ((index<0) || (index>=raidList.count()))
    {
        RTI->log("WARNING: Invalid raid index requested (Index = "+QString::number(index)+")");
        return 0;
    }

    return raidList.at(index);
}


inline rdsExportEntry* rdsRaid::getFirstExportEntry()
{
    if (exportList.count()==0)
    {
        return 0;
    }

    return exportList.at(0);
}


inline rdsRaidEntry* rdsRaid::getFirstRaidEntry()
{
    // Get the scan from the front of the export list
    rdsExportEntry* exportEntry=getFirstExportEntry();

    if (exportEntry==0)
    {
        // Something is wrong, probably list is empty
        return 0;
    }

    return getRaidEntry(exportEntry->raidIndex);
}


inline void rdsRaid::addExportEntry(int raidIndex, int protIndex, QString protName, bool anonymize, bool adjustmentScans)
{
    rdsExportEntry* entry=new rdsExportEntry;

    entry->raidIndex=raidIndex;
    entry->protIndex=protIndex;
    entry->protName=protName;
    entry->anonymize=anonymize;
    entry->adjustmentScans=adjustmentScans;

    exportList.append(entry);
}


inline void rdsRaid::addRaidEntry(int fileID, int measID, QString protName, QString patName, qint64 size, qint64 sizeOnDisk, QDateTime creationTime)
{
    rdsRaidEntry* entry=new rdsRaidEntry;

    entry->fileID=fileID;
    entry->measID=measID;
    entry->protName=protName;
    entry->patName=patName;
    entry->size=size;
    entry->sizeOnDisk=sizeOnDisk;
    entry->creationTime=creationTime;

    raidList.append(entry);
}


inline void rdsRaid::addRaidEntry(rdsRaidEntry* source)
{
    rdsRaidEntry* entry=new rdsRaidEntry;

    entry->fileID=source->fileID;
    entry->measID=source->measID;
    entry->protName=source->protName;
    entry->patName=source->patName;
    entry->size=source->size;
    entry->sizeOnDisk=source->sizeOnDisk;
    entry->creationTime=source->creationTime;
    entry->closingTime=source->closingTime;

    raidList.append(entry);
}


inline void rdsRaid::removePrecedingSpace(QString& text)
{
    while ((text.length()!=0) && (text.at(0)==' '))
    {
        text.remove(0,1);
    }
}


inline void rdsRaid::chopDependingIDs(QString& text)
{
    int length=text.length();

    if (length==0)
    {
        return;
    }

    int colon=text.lastIndexOf(":");

    if (colon != length-3)
    {
        text.chop(length-(colon+3));
    }
}


inline bool rdsRaid::isPatchedRaidToolMissing()
{
    return patchedRaidToolMissing;
}


inline void rdsRaid::setIgnoreLPFI()
{
    ignoreLPFID=true;
}


inline void rdsRaid::setORTSystemName(QString name)
{
    ortSystemName=name;
}


inline bool rdsRaid::isScanActive()
{
    return scanActive;
}


#endif // RDS_RAID_H



