#include "rds_mailboxwindow.h"
#include "ui_rds_mailboxwindow.h"
#include <QtCore>

rdsMailboxWindow::rdsMailboxWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsMailboxWindow)
{
    ui->setupUi(this);
    ui->titleLabel->setStyleSheet("QLabel { background-color : #580F8B; color : white; }");
    ui->titleIcon->setStyleSheet("QLabel { background-color : #580F8B; color : white; }");

    setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
    QRect screenrect = ((QGuiApplication*)QGuiApplication::instance())->primaryScreen()->availableGeometry();
    move(screenrect.right() - width(), screenrect.bottom() - height());
}

void rdsMailboxWindow::setMessage(QString message) {
    ui->titleLabel->setStyleSheet("QLabel { background-color : #580F8B; color : white; }");
    ui->titleIcon->setStyleSheet("QLabel { background-color : #580F8B; color : white; }");
    ui->buttonBox->clear();
    ui->buttonBox->addButton(QDialogButtonBox::StandardButton::Ok);
    ui->buttonBox->addButton(QDialogButtonBox::StandardButton::Cancel);
    ui->messageText->setText(message);
}

void rdsMailboxWindow::setError(QString message) {
    ui->titleLabel->setStyleSheet("QLabel { background-color : #FF0000; color : white; }");
    ui->titleIcon->setStyleSheet("QLabel { background-color : #FF0000; color : white; }");
    ui->buttonBox->clear();
    ui->buttonBox->addButton(QDialogButtonBox::StandardButton::Ok);
    ui->messageText->setText(message);
}

rdsMailboxWindow::~rdsMailboxWindow()
{
    delete ui;
}
//void rdsMailboxWindow::closeEvent( QCloseEvent* event )
//{
//    emit closing(buttonClicked);
//    event->accept();
//    destroy();
//}
void rdsMailboxWindow::keyPressEvent(QKeyEvent *e) {
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}

void rdsMailboxWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    qDebug() << "button clicked " << button->text();
    buttonClicked = button->text();
    emit closing(buttonClicked);
}
