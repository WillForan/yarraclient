#include "rds_mailboxwindow.h"
#include "ui_rds_mailboxwindow.h"
#include <QtCore>
#include <QtWidgets>

rdsMailboxWindow::rdsMailboxWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::rdsMailboxWindow)
{
    ui->setupUi(this);
    ui->titleLabel->setStyleSheet("QLabel { background-color : #580F8B; color : white; margin-left: 0px; padding-top: 9px; padding-bottom: 9px; font-size: 14px; }");
    ui->titleIcon->setStyleSheet("QLabel { background-color : #580F8B; color : white; }");

    setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
    QRect screenrect = ((QGuiApplication*)QGuiApplication::instance())->primaryScreen()->availableGeometry();
    move(screenrect.right() - width(), screenrect.bottom() - height());
}


void rdsMailboxWindow::setMessage(rdsMailboxMessage message)
{
    QString color;
    if (message.color.length() == 0) {
        color = "#580F8B";
    } else if (! (message.color.startsWith("#") && message.color.length() == 7 && !message.color.contains(";"))) {
            color = "#580F8B";
    } else {
        color = message.color;
    }
    ui->titleLabel->setStyleSheet(QString() + "QLabel { background-color : "+ color + "; color : white; margin-left: 0px; padding-top: 9px; padding-bottom: 9px; font-size: 14px; }");
    ui->titleIcon->setStyleSheet(QString() + "QLabel { background-color : "+ color + "; color : white; }");
    ui->buttonBox->clear();
    if (message.buttons.length() == 0) {
        QPushButton *addedButton = ui->buttonBox->addButton("Confirm", QDialogButtonBox::ActionRole);
        addedButton->setMinimumWidth(100);
    } else {
        for (QString& button: message.buttons) {
            ui->buttonBox->addButton(button, QDialogButtonBox::ActionRole)->setMinimumWidth(100);
        }
    }
    ui->messageText->setText(message.content);
}


void rdsMailboxWindow::setError(QString message)
{
    ui->titleLabel->setStyleSheet("QLabel { background-color : #FF0000; color : white; margin-left: 0px; padding-top: 9px; padding-bottom: 9px; font-size: 14px; }");
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


void rdsMailboxWindow::keyPressEvent(QKeyEvent *e)
{
    if(e->key() != Qt::Key_Escape)
    {
        QDialog::keyPressEvent(e);
    }
}


void rdsMailboxWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    qDebug() << "button clicked " << button->text();
    buttonClicked = button->text();
    emit closing(buttonClicked);
}
