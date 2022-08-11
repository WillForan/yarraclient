#include <QScrollbar>

#include "yd_mainwindow.h"
#include "ui_yd_mainwindow.h"
#include "yd_global.h"

#include "yd_test_systeminfo.h"
#include "yd_test.h"


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

    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(timerCall()));
    ui->runButton->setText("Run Diagnostics");

    ui->issuesEdit->clear();
    ui->allResultsEdit->clear();
    ui->tabWidget->setCurrentIndex(0);
    ui->progressBar->setTextVisible(false);
    ui->progressBar->setVisible(false);

    composeTests();
}


ydMainWindow::~ydMainWindow()
{
    delete ui;
}


void ydMainWindow::composeTests()
{
    testRunner.testList.append(new ydTestSysteminfo);
    testRunner.testList.append(new ydTestSysteminfo);
    testRunner.testList.append(new ydTestSysteminfo);
    testRunner.testList.append(new ydTestSysteminfo);
    testRunner.testList.append(new ydTestSysteminfo);
    testRunner.testList.append(new ydTestSysteminfo);
    testRunner.testList.append(new ydTestSysteminfo);
    testRunner.testList.append(new ydTestSysteminfo);
}


void ydMainWindow::on_runButton_clicked()
{
    if (testRunner.isActive)
    {
        if (!testRunner.isTerminating)
        {
            testRunner.cancelTests();
            ui->runButton->setText("Terminating...");
            ui->runButton->setEnabled(false);
        }
    }
    else
    {
        ui->tabWidget->setCurrentIndex(1);
        ui->progressBar->setValue(0);
        ui->progressBar->setEnabled(true);
        ui->progressBar->setTextVisible(true);
        ui->progressBar->setVisible(true);
        ui->runButton->setText("Cancel");
        ui->issuesEdit->clear();
        ui->allResultsEdit->clear();
        testRunner.runTests();
        updateTimer.start(50);
    }
}


void ydMainWindow::timerCall()
{
    // Update UI
    ui->progressBar->setValue(testRunner.getPercentage());
    ui->allResultsEdit->setHtml(testRunner.results);
    ui->allResultsEdit->verticalScrollBar()->setValue(ui->allResultsEdit->verticalScrollBar()->maximum());

    if (!testRunner.isActive)
    {
        updateTimer.stop();
        ui->progressBar->setValue(0);
        ui->progressBar->setEnabled(false);
        ui->progressBar->setTextVisible(false);
        ui->progressBar->setVisible(false);
        ui->runButton->setText("Run Diagnostics");
        ui->runButton->setEnabled(true);
        ui->tabWidget->setCurrentIndex(0);
        ui->issuesEdit->setText(testRunner.issues);

        ui->exportButton->setEnabled(true);
    }
}

void ydMainWindow::on_exportButton_clicked()
{
    //
}
