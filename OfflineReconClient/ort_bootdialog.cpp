#include "ort_bootdialog.h"
#include "ui_ort_bootdialog.h"


ortBootDialog::ortBootDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortBootDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags=Qt::Popup; 
    setWindowFlags(flags);

    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(255,106,19) );
    ui->progressBar->setPalette(p);
}


ortBootDialog::~ortBootDialog()
{
    delete ui;
}


void ortBootDialog::setFallbacktext()
{
    QString newText="Connecting to fallback server. Please wait...";
    ui->textLabel->setText(newText);
}
