#include "ort_copydialog.h"
#include "ui_ort_copydialog.h"

#include <QDesktopWidget>


ortCopyDialog::ortCopyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortCopyDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(255,106,19) );
    ui->progressBar->setPalette(p);

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignBottom, size(), qApp->desktop()->availableGeometry()));

}


ortCopyDialog::~ortCopyDialog()
{
    delete ui;
}
