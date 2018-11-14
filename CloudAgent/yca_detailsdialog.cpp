#include "yca_detailsdialog.h"
#include "ui_yca_detailsdialog.h"

#include <QtGui>
#include <QtWidgets>
#include <QKeyEvent>
#include <QMessageBox>
#include <QClipboard>


ycaDetailsDialog::ycaDetailsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ycaDetailsDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    QPalette newPalette=QPalette(QColor(240,240,240),QColor(240,240,240));
    newPalette.setColor(QPalette::Highlight, QColor(0,0,0));
    ui->tableWidget->setPalette(newPalette);
}


ycaDetailsDialog::~ycaDetailsDialog()
{
    delete ui;
}


#define YCA_ADDROW(a,b,c) ui->tableWidget->setRowCount(rowIndex+1); \
                          if (!QString(a).isEmpty()) { color=QColor(247,176,44); textcolor=QColor("#FFF"); } else { color=QColor("#FFF"); textcolor=QColor("#000"); } \
                          item=new QTableWidgetItem(a); \
                          item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); \
                          item->setBackgroundColor(color); item->setTextColor(textcolor); \
                          item->setFont(labelFont); \
                          ui->tableWidget->setItem(rowIndex,0,item); \
                          item=new QTableWidgetItem(b); \
                          item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter); \
                          item->setBackgroundColor(color); \
                          ui->tableWidget->setItem(rowIndex,1,item); \
                          item=new QTableWidgetItem(c); \
                          item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter); \
                          item->setBackgroundColor(color); \
                          ui->tableWidget->setItem(rowIndex,2,item); \
                          rowIndex++;

void ycaDetailsDialog::setTaskDetails(ycaTask* task)
{
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Category"));
    ui->tableWidget->setHorizontalHeaderItem(1,new QTableWidgetItem("Field"));
    ui->tableWidget->setHorizontalHeaderItem(2,new QTableWidgetItem("Value"));
    ui->tableWidget->horizontalHeader()->resizeSection(0,160);
    ui->tableWidget->horizontalHeader()->resizeSection(1,90);

    int rowIndex=0;
    ui->tableWidget->setRowCount(0);
    QTableWidgetItem* item=0;
    QColor color;
    QColor textcolor;

    QFont labelFont=ui->tableWidget->horizontalHeaderItem(0)->font();
    labelFont.setBold(true);

    YCA_ADDROW("Patient Information", "", "");
    YCA_ADDROW("", "Name",          task->patientName);
    YCA_ADDROW("", "Date of birth", task->dob);
    YCA_ADDROW("", "MRN #",         task->mrn);
    YCA_ADDROW("", "ACC #",         task->acc);
    YCA_ADDROW("Task Information",  "", "");
    YCA_ADDROW("", "Mode",          task->reconMode);
    YCA_ADDROW("", "UUID",          task->uuid);
    YCA_ADDROW("", "TaskID",        task->taskID);
    YCA_ADDROW("", "Status",        task->getStatus());
    YCA_ADDROW("", "Result",        task->getResult());
    YCA_ADDROW("", "Cost",          "$ "+QString::number(task->cost,'g',2));

    YCA_ADDROW("Performance",  "", "");
    if ((task->timeptCreated.isValid()) && (task->timeptCompleted.isValid()))
    {
        YCA_ADDROW("", "Total",      getTimeDiff(&task->timeptCreated, &task->timeptCompleted));
    }
    if ((task->timeptUploadBegin.isValid()) && (task->timeptUploadEnd.isValid()))
    {
        YCA_ADDROW("", "Upload",     getTimeDiff(&task->timeptUploadBegin, &task->timeptUploadEnd));
    }
    if ((task->timeptProcessingCreated.isValid()) && (task->timeptProcessingEnd.isValid()))
    {
        YCA_ADDROW("", "Processing", getTimeDiff(&task->timeptProcessingCreated, &task->timeptProcessingEnd));
    }
    if ((task->timeptDownloadBegin.isValid()) && (task->timeptDownloadEnd.isValid()))
    {
        YCA_ADDROW("", "Download",   getTimeDiff(&task->timeptDownloadBegin, &task->timeptDownloadEnd));
    }
    if ((task->timeptStorageBegin.isValid()) && (task->timeptStorageEnd.isValid()))
    {
        YCA_ADDROW("", "Storage",    getTimeDiff(&task->timeptStorageBegin, &task->timeptStorageEnd));
    }

    YCA_ADDROW("Task Log", "", "");
    if (task->timeptCreated.isValid())        { YCA_ADDROW("", "Created",        task->timeptCreated.toString());       }
    if (task->timeptUploadBegin.isValid())    { YCA_ADDROW("", "Upload Begin",   task->timeptUploadBegin.toString());   }
    if (task->timeptUploadEnd.isValid())      { YCA_ADDROW("", "Upload End",     task->timeptUploadEnd.toString());     }
    if (task->timeptDownloadBegin.isValid())  { YCA_ADDROW("", "Download Begin", task->timeptDownloadBegin.toString()); }
    if (task->timeptDownloadEnd.isValid())    { YCA_ADDROW("", "Download End",   task->timeptDownloadEnd.toString());   }
    if (task->timeptStorageBegin.isValid())   { YCA_ADDROW("", "Storage Begin",  task->timeptStorageBegin.toString());  }
    if (task->timeptStorageEnd.isValid())     { YCA_ADDROW("", "Storage End",    task->timeptStorageEnd.toString());    }
    if (task->timeptCompleted.isValid())      { YCA_ADDROW("", "Completed",      task->timeptCompleted.toString());     }
}


void ycaDetailsDialog::on_closeButton_clicked()
{
    this->close();
}


void ycaDetailsDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->type() == QKeyEvent::KeyPress)
    {
        if(event->matches(QKeySequence::Copy))
        {
            event->accept();

            int startLine=0;
            int endLine=ui->tableWidget->rowCount();

            if (!ui->tableWidget->selectedRanges().empty())
            {
                startLine=ui->tableWidget->selectedRanges().at(0).topRow();
                endLine=ui->tableWidget->selectedRanges().at(0).bottomRow()+1;
            }

            QString content="";

            for (int i=startLine; i<endLine; i++)
            {
                for (int j=0; j<ui->tableWidget->columnCount(); j++)
                {
                    QTableWidgetItem* item=ui->tableWidget->item(i,j);
                    if (item!=0)
                    {
                        content+=item->text();
                        if (j!=ui->tableWidget->columnCount()-1)
                        {
                            if (j==0)
                            {
                                content+="  ";
                            }
                            else
                            {
                                if (!item->text().isEmpty())
                                {
                                    content+=": ";
                                }
                            }
                        }
                        else
                        {
                            content+="\n";
                        }
                    }
                    else
                    {
                        content+="\n";
                        break;
                    }
                }
            }

            QClipboard* clipboard=QApplication::clipboard();
            clipboard->setText(content);
        }
    }
}


QString ycaDetailsDialog::getTimeDiff(QDateTime* start, QDateTime* end)
{
    QString text="";

    int secs=start->msecsTo(*end)/1000.;
    if (secs>=86400)
    {
        text+=QString::number(int(secs/86400.))+" days + ";
    }
    text+=QDateTime::fromTime_t(secs).toUTC().toString("hh:mm:ss");

    return text;
}


