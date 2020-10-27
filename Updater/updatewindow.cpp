#include "updatewindow.h"
#include "ui_updatewindow.h"

UpdateWindow::UpdateWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::UpdateWindow)
{
    ui->setupUi(this);

    QDir update_dir(updater.updateFolder);
    QStringList dirs = update_dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable);

    QIcon icon = QIcon(":/images/updateicon_16.png");

    setWindowIcon(icon);

    std::sort(dirs.begin(), dirs.end());

    for (int i = 0; i < dirs.size(); ++i) {
        QString err;
        QListWidgetItem *item = new QListWidgetItem(dirs[i]);
        if ( !rdsUpdater::isValidPackage(updater.updateFolder + "/" + dirs[i],err) ) {
           item->setFlags(item->flags() & ~Qt::ItemIsEnabled );
        }
        ui->updateVersionsWidget->addItem(item);
    }
    ui->updateProgressBar->setVisible(false);
}


void UpdateWindow::on_doUpdateButton_clicked()
{
    if (!ui->updateVersionsWidget->currentItem()) {
        return;
    }

    QString updateVersion = ui->updateVersionsWidget->currentItem()->text();
    QString error;
    ui->doUpdateButton->setEnabled(false);
    ui->updateModeSet->setEnabled(false);
    bool updated = updater.doVersionUpdate(updateVersion, error, ui->updateProgressBar);
    if (updated) {
        ui->updateProgressBar->setValue(100);
//        RTI->log(QString("Update to %1 applied").arg(updateVersion));
        QMessageBox::information(this,"Update","Update applied, restarting RDS...");

        // Shut this process down and start the updated .exe
        if (! QProcess::startDetached(qApp->applicationDirPath() + "/RDS.exe",{"-version-update"},qApp->applicationDirPath()) ) {
            QMessageBox::critical(this,"Reload failed", "Failed to launch new RDS. Installation may be corrupted. Shutting down: manual intervention required.");
        }
        qApp->quit();
    } else if (error.size() != 0) {
        qCritical().noquote() << "Update failed.";
//        RTI->log("Update failed: "+error);
        QMessageBox::critical(this,"Update failed", error);
        ui->updateProgressBar->setVisible(false);
    }
    ui->doUpdateButton->setEnabled(true);
    ui->updateModeSet->setEnabled(true);
}

void UpdateWindow::on_updateVersionsWidget_currentRowChanged(int currentRow)
{
//    QString compilation_date = QStringLiteral(__DATE__);
    (void)currentRow;
    QString updateVersion = ui->updateVersionsWidget->currentItem()->text();

    versionDetails details = updater.getVersionDetails(updateVersion);
    QDate compiledDate = QLocale("en_US").toDate(QStringLiteral(__DATE__).simplified(), "MMM d yyyy");

    QString displayString = QString("<h1>RDS version %1</h1> <br/> <b>Released on</b>: %2").arg(updateVersion, details.releaseDate.toString("yyyy-MM-dd"));
    if (compiledDate > details.releaseDate) {
        displayString += "<h3 style='color:red;'>Warning: this release is older than the current one.</h3>";
    }
    if(details.releaseNotes.size()) {
        displayString += QString("<p><b>Release notes</b>:</p> <p>%1</p>").arg(details.releaseNotes);
    }
    ui->updateVersionInfoDisplay->setText(displayString);
}

void UpdateWindow::on_updateModeSet_stateChanged(int set)
{
    ui->doUpdateButton->setEnabled(set!=0);
}

UpdateWindow::~UpdateWindow()
{
    delete ui;
}

