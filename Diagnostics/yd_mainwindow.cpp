#include "yd_mainwindow.h"
#include "ui_yd_mainwindow.h"
#include "yd_global.h"


ydMainWindow::ydMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ydMainWindow)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    setWindowTitle("Yarra - Diagnostics");
    setWindowIcon(YD_ICON);

    QString versionText = QString("<html><head/><body><p><span style=\" font-size:10pt; font-weight:600;\">Yarra Diagnostics</span><span style=\" font-size:10pt;\"><br/>Version %1</span></p></body></html>").arg(YD_VERSION);
    ui->versionLabel->setText(versionText);

    ui->runButton->setFocus();

    connect(&runTimer, SIGNAL(timeout()), this, SLOT(timerCall()));
    ui->runButton->setText("Run Diagnostics");

    QSize currentButtonSize = ui->runButton->size();
    ui->runButton->setMinimumSize(currentButtonSize);

    ui->issuesEdit->clear();
    ui->allResultsEdit->clear();
    ui->tabWidget->setCurrentIndex(0);
}


ydMainWindow::~ydMainWindow()
{
    delete ui;
}


void ydMainWindow::on_runButton_clicked()
{
    if (runTimer.isActive())
    {
        runTimer.stop();
        ui->progressBar->setValue(0);
        ui->progressBar->setEnabled(false);
        ui->runButton->setText("Run Diagnostics");
    }
    else
    {
        ui->progressBar->setValue(0);
        ui->progressBar->setEnabled(true);
        runTimer.start(30);
        ui->runButton->setText("Cancel");
        ui->issuesEdit->clear();
        ui->allResultsEdit->clear();
    }
}


void ydMainWindow::timerCall()
{
    int currentValue = ui->progressBar->value();
    if (currentValue==100)
    {
        runTimer.stop();
        ui->progressBar->setValue(0);
        ui->progressBar->setEnabled(false);
        ui->runButton->setText("Run Diagnostics");

        ui->issuesEdit->setText("Everything looks great!");
        ui->tabWidget->setCurrentIndex(0);
    }
    else
    {
        currentValue++;
        ui->progressBar->setValue(currentValue);
    }
}
