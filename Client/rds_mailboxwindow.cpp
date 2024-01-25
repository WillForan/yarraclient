#include "rds_mailboxwindow.h"
#include "ui_rds_mailboxwindow.h"
#include <QtCore>

rdsMailboxWindow::rdsMailboxWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsMailboxWindow)
{
    ui->setupUi(this);
    ui->titleLabel->setStyleSheet("QLabel { background-color : #580F8B; color : white; }");
    setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
}

void rdsMailboxWindow::setMessage(QString message) {
    ui->messageText->setText(message);
}

rdsMailboxWindow::~rdsMailboxWindow()
{
    delete ui;
}
void rdsMailboxWindow::closeEvent( QCloseEvent* event )
{
    emit closing(buttonClicked);
    event->accept();
    destroy();
}
void rdsMailboxWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    qDebug() << "button clicked " << button->text();
    buttonClicked = button->text();
    close();
}
