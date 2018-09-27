#include "sac_bootdialog.h"
#include "ui_sac_bootdialog.h"


sacBootDialog::sacBootDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sacBootDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags=Qt::Popup;
    setWindowFlags(flags);

    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(67,176,42));
    ui->progressBar->setPalette(p);
}


sacBootDialog::~sacBootDialog()
{
    delete ui;
}


void sacBootDialog::setText(QString text)
{
    ui->textLabel->setText(text);
}

