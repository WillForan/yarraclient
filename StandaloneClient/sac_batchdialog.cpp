#include "sac_batchdialog.h"
#include "ui_sac_batchdialog.h"

void sacBatchDialog::prepare(QList<ortModeEntry*> modes,QString notification) {
    ui->notificationEdit->setText(notification);
    modesInfo = modes;
    for (ortModeEntry* mode: modesInfo) {
        ui->modeComboBox->addItem(mode->readableName);
    }
}

sacBatchDialog::sacBatchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sacBatchDialog)
{
    ui->setupUi(this);
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

void sacBatchDialog::on_removeButton_clicked()
{

}

void sacBatchDialog::on_addFileButton_clicked()
{

    // Add button clicked
    // Adding at the end

    // Get the position
    int row = files->rowCount();

    // Enable add one or more rows
    files->insertRows(row,1);
    QString newFilename=QFileDialog::getOpenFileName(this, "Select Measurement File...", QString(), "TWIX rawdata (*.dat);;Anything (*)");

    files->setData(files->index(row),newFilename);
    // Get the row for Edit mode
    QModelIndex index = files->index(row);

    // Enable item selection and put it edit mode
    ui->filesListView->setCurrentIndex(index);

//    ui->listView->set(index);
}

void sacBatchDialog::on_removeFileButton_clicked()
{
    files->removeRows(ui->filesListView->currentIndex().row(),1);
}

void sacBatchDialog::on_buttonBox_accepted()
{

}

void sacBatchDialog::on_addModeButton_clicked()
{
    int row = modes->rowCount();

    modes->insertRow(row);
    ortModeEntry* mode = modesInfo.at(ui->modeComboBox->currentIndex());
    modes->setData(modes->index(row),mode->idName);
}

void sacBatchDialog::on_importBatchFileButton_clicked()
{
    QString newFilename=QFileDialog::getOpenFileName(this, "Select Batch File...", QString(), "Batch files (*.sac);;Anything (*)");
    if (newFilename.length()) {
        QFileInfo fileInfo = QFileInfo(newFilename);
        QSettings config(fileInfo.absoluteFilePath(), QSettings::IniFormat);
        files->setStringList( QStringList{} );
        modes->setStringList( QStringList{} );

        int i=0;
        while (1)
        {
            QString scan = QString("Scans/Scan") + QString::number(i);
            if (!config.contains(scan)) {
                break;
            }
            scan=config.value(scan,"" ).toString();

            int row = files->rowCount();
            files->insertRows(row,1);
            files->setData(files->index(row),scan);
            i++;
        }
        i=0;
        while (1)
        {
            QString mode = QString("ReconModes/Mode") + QString::number(i);
            if (!config.contains(mode)) {
                break;
            }
            mode=config.value(mode,"" ).toString();

            int row = modes->rowCount();
            modes->insertRows(row,1);
            modes->setData(modes->index(row),mode);
            i++;
        }
    }
}

void sacBatchDialog::on_exportBatchFileButton_clicked()
{
    QString newFilename=QFileDialog::getSaveFileName(this, "Save batch file...", QString(), "Batch files (*.sac)");
    QFileInfo fileInfo = QFileInfo(newFilename);
    QSettings config(fileInfo.absoluteFilePath(), QSettings::IniFormat);
    config.beginGroup("ReconModes");
    int i=0;
    for (auto mode: modes->stringList()) {
        config.setValue("Mode"+QString::number(i),mode);
        i++;
    }
    config.endGroup();
    config.beginGroup("Scans");
    i=0;
    for (auto scan: files->stringList()) {
        config.setValue("Scan"+QString::number(i),scan);
        i++;
    }

}

void sacBatchDialog::on_removeModeButton_clicked()
{
    modes->removeRows(ui->modesListView->currentIndex().row(),1);
}
