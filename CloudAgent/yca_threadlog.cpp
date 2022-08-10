#include "yca_threadlog.h"
#include "yca_global.h"

ycaThreadLog* ycaThreadLog::pSingleton=0;


ycaThreadLog* ycaThreadLog::getInstance()
{
    if (pSingleton==0)
    {
        pSingleton=new ycaThreadLog();
    }
    return pSingleton;
}


ycaThreadLog::ycaThreadLog()
{
#ifndef YTL_DISABLED
    QDir appDir(qApp->applicationDirPath());
    if (!appDir.exists("/log"))
    {
        appDir.mkdir("./log");
    }

    QString logFilename=qApp->applicationDirPath()+"/log/yca.log";

    logfile.setFileName(logFilename);
    logfile.open(QIODevice::Append | QIODevice::Text);

    if (logfile.size() > YTL_MAXLOGSIZE)
    {
        log("Size of logfile getting large.");
        log("Renaming file and creating empty file.");
        logfile.flush();

        if (!logfile.rename(logFilename + "_" + QDateTime::currentDateTime().toString("ddMMyyHHmmss")))
        {
            // TODO: Error handling
        }

        logfile.close();
        logfile.setFileName(logFilename);
        logfile.open(QIODevice::Append | QIODevice::Text);
    }

    logfile.write(QString("---\n").toLatin1());
    QString startEntry=formatLine("Yarra Cloud Agent started (V "+QString(YCA_VERSION)+")", YTL_INFO, YTL_HIGH);
    logfile.write(startEntry.toLatin1());
    logfile.flush();
#endif
}


#ifndef YTL_DISABLED

void ycaThreadLog::readLogFile(QTableWidget* widget, int detailLevel)
{
    QTableWidgetItem* item=0;

    widget->clearContents();
    widget->setColumnCount(5);

    item=new QTableWidgetItem("Time");
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    widget->setHorizontalHeaderItem(0,item);
    widget->setHorizontalHeaderItem(1,new QTableWidgetItem("Type"));
    widget->setHorizontalHeaderItem(2,new QTableWidgetItem("Level"));
    widget->setHorizontalHeaderItem(3,new QTableWidgetItem("Thread"));

    item=new QTableWidgetItem("Description");
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    widget->setHorizontalHeaderItem(4,item);

    widget->horizontalHeader()->resizeSection(0,110);
    widget->horizontalHeader()->resizeSection(1,60);
    widget->horizontalHeader()->resizeSection(2,60);
    widget->horizontalHeader()->resizeSection(3,60);
    widget->horizontalHeader()->resizeSection(4,100);

    QStringList fileContent;
    QFile logfile;
    logfile.setFileName(qApp->applicationDirPath()+"/log/yca.log");
    logMutex.lock();
    logfile.open(QIODevice::ReadOnly | QIODevice::Text);
    while (!logfile.atEnd())
    {
         fileContent << QString(logfile.readLine());
    }
    logfile.close();
    logMutex.unlock();

    /*
    for (int i=0; i<10000; i++)
    {
        fileContent << QString(fileContent.at(10));
    }
    */

    int renderedLines=0;
    for (int i=fileContent.count()-1; i>=0; i--)
    {
        QStringList entries=fileContent.at(i).split(YTL_SEP);

        QColor rowColor("#D9D9D9");
        QColor textColor(35, 35, 35);
        QString typeStr="INFO";
        QString levelStr="MID";
        ImportanceLevel level=Medium;
        EntryType        type=Info;

        if (entries.count()<5)
        {
            // Separator or incomplete entry
            widget->setRowCount(renderedLines+1);
        }
        else
        {
            if (entries.at(2)=="L")
            {
                levelStr="LOW";
                level=Low;
            }
            if (entries.at(2)=="H")
            {
                levelStr="HIGH";
                level=High;
            }

            if (entries.at(1)=="E")
            {
                typeStr="ERROR";
                rowColor=QColor("#FF7270");
                type=Error;
            }
            if (entries.at(1)=="W")
            {
                typeStr="WARN";
                rowColor=QColor("#FFD177");
                type=Warning;
            }

            if ((type==Info) && (level==High))
            {
                rowColor="#9BD5FF";
            }

            if ((int) level > detailLevel)
            {
                continue;
            }

            widget->setRowCount(renderedLines+1);

            item=new QTableWidgetItem(entries.at(0));
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            item->setBackgroundColor(rowColor);
            item->setTextColor(textColor);
            widget->setItem(renderedLines,0,item);

            item=new QTableWidgetItem(typeStr);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            item->setBackgroundColor(rowColor);
            item->setTextColor(textColor);
            widget->setItem(renderedLines,1,item);

            item=new QTableWidgetItem(levelStr);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            item->setBackgroundColor(rowColor);
            item->setTextColor(textColor);
            widget->setItem(renderedLines,2,item);

            item=new QTableWidgetItem(entries.at(3));
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            item->setBackgroundColor(rowColor);
            item->setTextColor(textColor);
            widget->setItem(renderedLines,3,item);

            item=new QTableWidgetItem(entries.at(4));
            // Remove trailing \n char for tooltip
            QString toolTipStr=entries.at(4);
            toolTipStr.chop(1);
            item->setToolTip(toolTipStr);
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            item->setBackgroundColor(rowColor);
            item->setTextColor(textColor);
            widget->setItem(renderedLines,4,item);
        }
        renderedLines++;

        if (renderedLines>YTL_MAXLOGLINES)
        {
            break;
        }
    }

    if (renderedLines>YTL_MAXLOGLINES)
    {
        widget->setRowCount(renderedLines+1);

        item=new QTableWidgetItem("... To see previous entries, open log externally ...");
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        //item->setBackgroundColor(rowColor);
        widget->setItem(renderedLines,4,item);
    }
}


QString ycaThreadLog::getClipboardString(QTableWidget* widget)
{
    int startLine=0;
    int endLine=widget->rowCount();

    if (!widget->selectedRanges().empty())
    {
        startLine=widget->selectedRanges().at(0).topRow();
        endLine=widget->selectedRanges().at(0).bottomRow()+1;
    }

    QString content="";

    for (int i=startLine; i<endLine; i++)
    {
        for (int j=0; j<widget->columnCount(); j++)
        {
            QTableWidgetItem* item=widget->item(i,j);
            if (item!=0)
            {
                content+=item->text();
                if (j!=widget->columnCount()-1)
                {
                    content+=YTL_SEP;
                }
            }
            else
            {
                content+="---\n";
                break;
            }
        }
    }

    return content;
}

#endif

