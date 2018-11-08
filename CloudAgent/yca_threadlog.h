#ifndef YCATHREADLOG_H
#define YCATHREADLOG_H

#include <QtGui>
#include <QtWidgets>

// Disable the thread logger for the other Yarra clients
#ifdef YARRA_APP_SAC
    #define YTL_DISABLED 1
#endif
#ifdef YARRA_APP_ORT
    #define YTL_DISABLED 1
#endif


#define YTL ycaThreadLog::getInstance()
#define YTL_MAXLOGSIZE  1000000
#define YTL_MAXLOGLINES 10000

#define YTL_SEP "|"

#define YTL_INFO    ycaThreadLog::Info
#define YTL_WARNING ycaThreadLog::Warning
#define YTL_ERROR   ycaThreadLog::Error

#define YTL_LOW     ycaThreadLog::Low
#define YTL_MID     ycaThreadLog::Medium
#define YTL_HIGH    ycaThreadLog::High

#define YTL_PRIVACY(T) "{"+T+"}"


class ycaThreadLog
{
public:

    enum EntryType
    {
        Info=0,
        Warning,
        Error
    };

    enum ImportanceLevel
    {
        High=0,
        Medium,
        Low
    };

    ycaThreadLog();
    static ycaThreadLog* getInstance();

    void log(QString text, EntryType type=Info, ImportanceLevel level=Medium);

#ifndef YTL_DISABLED
    void readLogFile(QTableWidget* widget, int detailLevel);
    QString getClipboardString(QTableWidget* widget);

    void lock();
    void unlock();
#endif

protected:

    static ycaThreadLog* pSingleton;

#ifndef YTL_DISABLED
    QString formatLine(QString text, EntryType type, ImportanceLevel level);
    QString getEntryType(EntryType type);
    QString getDetailLevel(ImportanceLevel level);

    QFile  logfile;
    QMutex logMutex;
#endif
};


inline void ycaThreadLog::log(QString text, EntryType type, ImportanceLevel level)
{
#ifndef YTL_DISABLED
    QString line=formatLine(text,type,level);
    logMutex.lock();
    logfile.write(line.toLatin1());
    logfile.flush();
    logMutex.unlock();

    qInfo() << line;
#endif
}


#ifndef YTL_DISABLED

inline QString ycaThreadLog::getEntryType(EntryType type)
{
    switch (type)
    {
    case Info:
        return "I";
        break;

    case Warning:
        return "W";
        break;

    case Error:
        return "E";
        break;

    default:
        return "N";
        break;
    }
}


inline QString ycaThreadLog::getDetailLevel(ImportanceLevel level)
{
    switch (level)
    {
    case Low:
        return "L";
        break;

    case Medium:
        return "M";
        break;

    case High:
        return "H";
        break;

    default:
        return "N";
        break;
    }
}


inline QString ycaThreadLog::formatLine(QString text, EntryType type, ImportanceLevel level)
{
    return QDateTime::currentDateTime().toString("dd.MM.yy  hh:mm:ss")
           + YTL_SEP + getEntryType(type)
           + YTL_SEP + getDetailLevel(level)
           + YTL_SEP + QString::number((int)QThread::currentThreadId())
           + YTL_SEP + text
           + "\n";
}


inline void ycaThreadLog::lock()
{
    logMutex.lock();
}


inline void ycaThreadLog::unlock()
{
    logMutex.unlock();
}

#endif


#endif // YCATHREADLOG_H

