#include "rds_mailbox.h"
#include <QGuiApplication>
#include "rds_network.h"
MailboxMessage::MailboxMessage() {}

MailboxMessage::MailboxMessage(QString id, QString content) {
    this->id=id;
    this->content;
}


Mailbox::Mailbox(QObject *parent) : QObject(parent) {
//    worker.moveToThread(&mailboxThread);
//    mailboxThread.start();
}

MailboxMessage::MailboxMessage(QJsonObject obj) {
    id=obj.value("message_id").toString();
    content=obj.value("content").toString();
}


MailboxWorker::MailboxWorker(QObject *parent) : QObject(parent)
{
}
void MailboxWorker::start() {
    if (!RTI_NETLOG.isConfigured()) {
        qDebug() << "netlog not configured";
        return;
    }
    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(updateMailbox()));
    QObject::connect(this, SIGNAL(newMessage(MailboxMessage)), this, SLOT(showMessage(MailboxMessage)));
    timer.start(1000);
}

void MailboxWorker::onMarkedMessageResponse()
{
    timer.start(1000);
    if (currentReply->error() == QNetworkReply::NetworkError::NoError)
    {
        qDebug() << "Marked message response" << currentReply->readAll();
        mailboxWindow->close();
        return;
    }
    mailboxWindow = new rdsMailboxWindow();
    mailboxWindow->setError("Network error while responding to message");
    mailboxWindow->show();
    delete currentReply;
    currentReply = nullptr;
}

void MailboxWorker::onNewMessagesResponse()
{
    timer.start(1000);
    if (currentReply->error() != QNetworkReply::NetworkError::NoError) {
        qDebug()<< "Network error:" << currentReply->errorString();
        return;
    }
    auto result = currentReply->readAll();
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



void MailboxWorker::windowClosing(QString button) {
    qDebug()<< "Window closing" << button;
    QUrlQuery query;
    query.addQueryItem("source_id",     RTI_CONFIG->infoSerialNumber);
    currentReply = RTI_NETLOG.postDataAsync(query, QString("mark_message_as_read/") + currentMessage.id);
    if (!currentReply) {
        return;
    }
    connect(currentReply, SIGNAL(finished()), this, SLOT(onMarkedMessageResponse())) ;
    timeout.singleShot(2000, this, &MailboxWorker::replyTimeout);
}

void MailboxWorker::showMessage(MailboxMessage message) {
    qDebug()<< message.id;
    timer.stop();
    mailboxWindow = new rdsMailboxWindow();
//    QRect screenrect = ((QGuiApplication*)QGuiApplication::instance())->primaryScreen()->availableGeometry();
//    mailboxWindow->move(screenrect.right() - mailboxWindow->width(), screenrect.bottom() - mailboxWindow->height());
    mailboxWindow->setMessage(message.content);
    connect(mailboxWindow, SIGNAL(closing(QString)), this, SLOT(windowClosing(QString)));
    mailboxWindow->show();
    currentMessage = message;
}

void MailboxWorker::replyTimeout() {
    if (currentReply->isRunning()) {
        qDebug() << "Aborting: timeout";
        emit currentReply->abort();
    }
}

void MailboxWorker::updateMailbox(){
    qDebug()<<"Worker::updateMailbox called";
    QUrlQuery query;
    query.addQueryItem("source_id",     RTI_CONFIG->infoSerialNumber);

    currentReply = RTI_NETLOG.postDataAsync(query, "get_unread_messages");
    if (!currentReply) {
        return;
    }
    connect(currentReply, SIGNAL(finished()), this, SLOT(onNewMessagesResponse())) ;
    timeout.singleShot(2000, this, &MailboxWorker::replyTimeout);
    timer.stop();
    //    netLogger.waitForReply(reply, 1000);
//    emit newMessage("asdf");
}
