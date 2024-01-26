#include "rds_mailbox.h"
#include <QGuiApplication>
#include "rds_network.h"
MailboxMessage::MailboxMessage() {}

MailboxMessage::MailboxMessage(QString id, QString content) {
    this->id=id;
    this->content;
}


Mailbox::Mailbox(QObject *parent) : QObject(parent) {
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
    QObject::connect(&timer, &QTimer::timeout, this, &MailboxWorker::updateMailbox);
    startChecking();
}

void MailboxWorker::startChecking() {
    qDebug() << "start checking";
    timer.start(1000);
}

void MailboxWorker::stopChecking() {
    qDebug() << "stop checking";
    timer.stop();
}

void MailboxWorker::updateMailbox(){
    qDebug()<<"Worker::updateMailbox called";

    doRequest("get_unread_messages", [this](QNetworkReply* reply) {
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

void MailboxWorker::windowClosing(QString button) {
    qDebug()<< "Window closing" << button;
    bool did = doRequest(QString("mark_message_as_read/") + currentMessage.id, [this](QNetworkReply* reply) {
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

void MailboxWorker::showMessage(MailboxMessage message) {
    qDebug()<< message.id;
    stopChecking();
    mailboxWindow = new rdsMailboxWindow();
    mailboxWindow->setMessage(message.content);
    currentMessage = message;
    connect(mailboxWindow, &rdsMailboxWindow::closing, this, &MailboxWorker::windowClosing);
    mailboxWindow->show();

}

template <typename F>
bool MailboxWorker::doRequest(QString endpoint, F&& fn) {
    QUrlQuery query;
    query.addQueryItem("api_key",   RTI_CONFIG->logApiKey);
    query.addQueryItem("source_id", RTI_CONFIG->infoSerialNumber);

    QNetworkReply* reply = RTI_NETLOG.postDataAsync(query, endpoint);
    if (!reply) {
        return false;
    }
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, fn, endpoint] {
                qDebug() << "Reply from: " << endpoint;
                (fn)(reply);
                reply->deleteLater();
            }
    );
    QTimer::singleShot(2000, reply,
        [this,reply]() {
            qDebug()<<"timeout";
            if (reply->isRunning()) {
                qDebug() << "Aborting: timeout";
                emit reply->abort();
            }
            reply->deleteLater();
        }
    );
    return true;
}

