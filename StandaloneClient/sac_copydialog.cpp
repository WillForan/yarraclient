#include "sac_copydialog.h"
#include "ui_sac_copydialog.h"

#include <QtWidgets>


sacCopyDialog::sacCopyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sacCopyDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(67,176,42) );
    ui->progressBar->setPalette(p);

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignBottom, size(), qApp->desktop()->availableGeometry()));
}


void sacCopyDialog::setText(QString text)
{
    ui->textLabel->setText(text);
}


sacCopyDialog::~sacCopyDialog()
{
    delete ui;
}

