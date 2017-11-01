#include "sac_batchdialog.h"
#include "ui_sac_batchdialog.h"
#include "sac_global.h"


sacBatchDialog::sacBatchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sacBatchDialog)
{
    mainWindow = (sacMainWindow*) parent;
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    setWindowTitle("Batch Processing");
    setWindowIcon(SAC_ICON);

    // Create model
    files = new QStringListModel(this);

    // Make data
    ui->filesListView->setModel(files);

    modes = new QStringListModel(this);
    ui->modesListView->setModel(modes);
}


sacBatchDialog::~sacBatchDialog()
{
    delete ui;
}


void sacBatchDialog::prepare(QList<ortModeEntry*> modes,QString notification)
{
    ui->notificationEdit->setText(notification);
    modesInfo = modes;

    for (ortModeEntry* mode: modesInfo)
    {
        ui->modeComboBox->addItem(mode->readableName);
    }
}


void sacBatchDialog::on_addFileButton_clicked()
{
    // Get the position
    int row = files->rowCount();

    // Enable add one or more rows
    QString newFilename=QFileDialog::getOpenFileName(this, "Select Measurement File...", QString(), "TWIX rawdata (*.dat);;Anything (*)");

    if (!newFilename.isEmpty())
    {
        files->insertRows(row,1);
        files->setData(files->index(row),newFilename);

        // Get the row for Edit mode
        QModelIndex index = files->index(row);

        // Enable item selection and put it edit mode
        ui->filesListView->setCurrentIndex(index);
    }
}


void sacBatchDialog::on_removeFileButton_clicked()
{
    files->removeRows(ui->filesListView->currentIndex().row(),1);
}


void sacBatchDialog::on_addModeButton_clicked()
{
    int row = modes->rowCount();

    modes->insertRow(row);
    ortModeEntry* mode = modesInfo.at(ui->modeComboBox->currentIndex());
    modes->setData(modes->index(row),mode->idName);
}


void sacBatchDialog::on_removeModeButton_clicked()
{
    modes->removeRows(ui->modesListView->currentIndex().row(),1);
}


void sacBatchDialog::on_importBatchFileButton_clicked()
{
    QString newFilename=QFileDialog::getOpenFileName(this, "Select Batch File...", QString(), "Batch files (*.sac);;Anything (*)");

    if (newFilename.length())
    {
        QStringList files_l; QStringList modes_l;
        QString notify;
        TaskPriority priority;
        mainWindow->readBatchFile(newFilename,files_l, modes_l, notify, priority);
        ui->notificationEdit->setText(notify);
        if ( priority == TaskPriority::Normal ) {
            ui->priorityCombobox->setCurrentIndex(0);
        } else if ( priority == TaskPriority::Night ) {
            ui->priorityCombobox->setCurrentIndex(1);
        } else if ( priority == TaskPriority::HighPriority ) {
            ui->priorityCombobox->setCurrentIndex(2);
        }
        files->setStringList( files_l );
        modes->setStringList( modes_l );
    }
}


void sacBatchDialog::on_exportBatchFileButton_clicked()
{
    QString newFilename=QFileDialog::getSaveFileName(this, "Save batch file...", QString(), "Batch files (*.sac)");
    QFileInfo fileInfo = QFileInfo(newFilename);
    QSettings config(fileInfo.absoluteFilePath(), QSettings::IniFormat);

    config.beginGroup("ReconModes");
    int i=0;
    for (auto mode: modes->stringList())
    {
        config.setValue("Mode"+QString::number(i),mode);
        i++;
    }
    config.endGroup();

    config.beginGroup("Scans");
    i=0;
    for (auto scan: files->stringList())
    {
        config.setValue("Scan"+QString::number(i),scan);
        i++;
    }
}
