#ifndef RDS_LOG_H
#define RDS_LOG_H

#include <QtGui>
#include <QWidget>
#include <QTextEdit>
#include <QString>


class rdsLog
{
public:
    rdsLog();

    void start();
    void finish();
    void log(QString text);
    void flush();
    void debug(QString text);

    void setLogWidget(QTextEdit* widget);
    void clearLogWidget();

    QString getLogFilename();
    QString getLocalLogPath();

    void pauseLogfile();
    void resumeLogfile();

protected:

    QFile logfile;
    QTextEdit* logWidget;

};



inline void rdsLog::setLogWidget(QTextEdit* widget)
{
    logWidget=widget;
}


inline void rdsLog::log(QString text)
{
    QString line=QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss") + "  --  " + text + "\n";
    logfile.write(line.toLatin1());
    logfile.flush();
    qInfo() << text;

    if (logWidget!=0)
    {
        logWidget->setPlainText(line+logWidget->toPlainText());
    }

}


inline void rdsLog::debug(QString text)
{
    QString line=QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss") + "  >>  " + text + "\n";

    if (logWidget!=0)
    {
        logWidget->setText(line+logWidget->toPlainText());
    }

}


inline void rdsLog::flush()
{
    logfile.flush();
}


#endif // RDS_LOG_H




