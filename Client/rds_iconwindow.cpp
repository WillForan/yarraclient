#include "rds_iconwindow.h"
#include "ui_rds_iconwindow.h"

#include <QDesktopWidget>


rdsIconWindow::rdsIconWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsIconWindow)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowStaysOnTopHint;
    flags |= Qt::Tool;
    setWindowFlags(flags);

    QPalette p = palette();
    p.setColor(QPalette::Background, QColor(0,0,0));
    this->setPalette(p);

    /*
    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(88,15,139));
    ui->progressBar->setPalette(p);
    */

    // Menubar of Syngo MR is 25 pixels high

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignTop, QSize(32,25), qApp->desktop()->availableGeometry()));


}


rdsIconWindow::~rdsIconWindow()
{
    delete ui;
}

