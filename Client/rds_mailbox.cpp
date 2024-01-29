#include "rds_mailbox.h"
#include <QGuiApplication>
#include "rds_network.h"
MailboxMessage::MailboxMessage() {}

MailboxMessage::MailboxMessage(QString id, QString content) {
    this->id=id;
    this->content;
}

MailboxMessage::MailboxMessage(QJsonObject obj) {
    id=obj.value("message_id").toString();
    content=obj.value("content").toString();
}

Mailbox::Mailbox(QObject *parent) : QObject(parent)
{
}
void Mailbox::start() {
    if (!RTI_NETLOG.isConfigured()) {
        qDebug() << "netlog not configured";
        return;
    }
    QObject::connect(&timer, &QTimer::timeout, this, &Mailbox::updateMailbox);
    startChecking();
}

void Mailbox::startChecking() {
    qDebug() << "start checking";
    timer.start(1000);
}

void Mailbox::stopChecking() {
    qDebug() << "stop checking";
    timer.stop();
}

void Mailbox::updateMailbox(){
    qDebug()<<"updateMailbox called";

    RTI_NETLOG.doRequest("get_unread_messages", [this](QNetworkReply* reply) {
        startChecking();

        if (reply->error() != QNetworkReply::NetworkError::NoError) {
            qDebug()<< "Network error:" << reply->errorString();
            return;
        }
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
        showMessage(MailboxMessage(unread_messages.at(0).toObject()));
    });
}

void Mailbox::windowClosing(QString button) {
    qDebug()<< "Window closing" << button;
    QUrlQuery query;
    query.addQueryItem("response", button);
    bool did = RTI_NETLOG.doRequest(QString("mark_message_as_read/") + currentMessage.id, query, [this](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NetworkError::NoError)
        {
            startChecking();
            qDebug() << "Marked message response" << reply->readAll();
            mailboxWindow->close();
            return;
        }
        mailboxWindow = new rdsMailboxWindow();
        mailboxWindow->setError("Network error while responding to message");
        connect(mailboxWindow, &rdsMailboxWindow::closing, this, [this](QString _){ this->startChecking(); });
        mailboxWindow->show();

    });
    if (did) {
        stopChecking();
    }
}

void Mailbox::showMessage(MailboxMessage message) {
    qDebug()<< message.id;
    stopChecking();
    mailboxWindow = new rdsMailboxWindow();
    mailboxWindow->setMessage(message.content);
    currentMessage = message;
    connect(mailboxWindow, &rdsMailboxWindow::closing, this, &Mailbox::windowClosing);
    mailboxWindow->show();

}

