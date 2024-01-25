#include "rds_mailbox.h"
#include <QGuiApplication>

MailboxMessage::MailboxMessage() {}

MailboxMessage::MailboxMessage(QString id, QString content) {
    this->id=id;
    this->content;
}

MailboxMessage::MailboxMessage(QJsonObject obj) {
    id=obj.value("message_id").toString();
    content=obj.value("content").toString();
}


MailboxWorker::MailboxWorker(QObject *parent) : QObject(parent)
{
    netLogger.configure("localhost:9999", EventInfo::SourceType::RDS, "12345", "12345", true);
    connect(netLogger.networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNewMessagesResponse(QNetworkReply*))) ;
}
void MailboxWorker::onMarkedMessageResponse(QNetworkReply* reply)
{
    qDebug() << "Marked message response";
    disconnect(netLogger.networkManager, SIGNAL(finished(QNetworkReply*)), 0,0);
    connect(netLogger.networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNewMessagesResponse(QNetworkReply*))) ;
    timer.start(1000);
}

void MailboxWorker::onNewMessagesResponse(QNetworkReply* reply)
{
    timer.start(1000);
    auto result = reply->readAll();
    QJsonParseError err;
    QJsonDocument js = QJsonDocument::fromJson(result, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << err.errorString();
        return;
    }
    QJsonArray unread_messages = js.object().value("unread_messages").toArray();
    if (unread_messages.count() == 0 ) {
        qDebug()<<"No unread messages";
        return;
    }
    emit newMessage(MailboxMessage(unread_messages.at(0).toObject()));

    qDebug()<<"API Response" << js.toJson();
}


Mailbox::Mailbox(QObject *parent) : QObject(parent) {
    QObject::connect(&worker.timer, SIGNAL(timeout()), &worker, SLOT(onTimeout()));
    QObject::connect(&worker, SIGNAL(newMessage(MailboxMessage)), this, SLOT(showMessage(MailboxMessage)));
    worker.timer.start(1000);
//    worker.moveToThread(&mailboxThread);
//    mailboxThread.start();
}

void MailboxWorker::windowClosing(QString button) {
    qDebug()<< "Window closed" << button;
    QUrlQuery query;
    disconnect(netLogger.networkManager, SIGNAL(finished(QNetworkReply*)), 0,0);
    connect(netLogger.networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onMarkedMessageResponse(QNetworkReply*))) ;
    netLogger.postDataAsync(query, QString("mark_message_as_read/") + currentMessage.id);
//    worker.timer.start(1000);
}

void Mailbox::showMessage(MailboxMessage message) {
    qDebug()<< message.id;
    worker.timer.stop();
    mailboxWindow = new rdsMailboxWindow();
    QRect screenrect = ((QGuiApplication*)QGuiApplication::instance())->primaryScreen()->availableGeometry();
    mailboxWindow->move(screenrect.right() - mailboxWindow->width(), screenrect.bottom() - mailboxWindow->height());
    mailboxWindow->setMessage(message.content);
    connect(mailboxWindow, SIGNAL(closing(QString)), &worker, SLOT(windowClosing(QString)));
    mailboxWindow->show();
    worker.currentMessage = message;
}


void MailboxWorker::onTimeout(){
    qDebug()<<"Worker::onTimeout called";
    QUrlQuery query;
    netLogger.postDataAsync(query, "get_unread_messages");
    timer.stop();
    //    netLogger.waitForReply(reply, 1000);
//    emit newMessage("asdf");
}
