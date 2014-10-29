#include "ort_waitdialog.h"
#include "ui_ort_waitdialog.h"


ortWaitDialog::ortWaitDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortWaitDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(255,106,19) );
    ui->progressBar->setPalette(p);
}

ortWaitDialog::~ortWaitDialog()
{
    delete ui;
}
