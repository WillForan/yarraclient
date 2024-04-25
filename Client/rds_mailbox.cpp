#include "rds_mailbox.h"
#include <QGuiApplication>
#include "rds_network.h"


rdsMailbox::rdsMailbox(QObject *parent) : QObject(parent)
{
}


void rdsMailbox::start()
{
    if (!RTI_NETLOG.isConfigured())
    {
        qDebug() << "netlog not configured";
        return;
    }
    QObject::connect(&timer, &QTimer::timeout, this, &rdsMailbox::updateMailbox);
    startChecking();
}


void rdsMailbox::startChecking()
{
    timer.start(checkInterval);
}


void rdsMailbox::stopChecking()
{
    qDebug() << "stop checking";
    timer.stop();
}


void rdsMailbox::updateMailbox()
{
    RTI_NETLOG.doRequest("mailbox/get_unread_messages", [this](QNetworkReply* reply)
    {
        startChecking();

        if (reply->error() != QNetworkReply::NetworkError::NoError)
        {
            qDebug()<< "Network error:" << reply->errorString();
            return;
        }
        auto result = reply->readAll();
        QJsonParseError err;
        QJsonDocument js = QJsonDocument::fromJson(result, &err);
        if (err.error != QJsonParseError::NoError)
        {
            qDebug() << err.errorString();
            return;
        }
        QJsonArray unread_messages = js.object().value("unread_messages").toArray();
        if (unread_messages.count() == 0 )
        {
            qDebug()<<"No unread messages";
            return;
        }
        showMessage(rdsMailboxMessage(unread_messages.at(0).toObject()));
    });
}


void rdsMailbox::windowClosing(QString button)
{
    qDebug()<< "Window closing" << button;
    QUrlQuery query;
    query.addQueryItem("response", button);
    bool did = RTI_NETLOG.doRequest(QString("mailbox/mark_message_as_read/") + currentMessage.id, query, [this](QNetworkReply* reply)
        {
            if (reply->error() == QNetworkReply::NetworkError::NoError)
            {
                startChecking();
                qDebug() << "Marked message response" << reply->readAll();
                mailboxWindow->close();
                errorCount = 0;
                return;
            }
            mailboxWindow->close();
            rdsMailboxWindow* errorWindow = new rdsMailboxWindow();
            errorCount++;
            if (errorCount > 3) {
                errorWindow->setError("Exceeded error threshold. Disabling mailbox until RDS is restarted.");
            } else {
                errorWindow->setError("Network error while responding to message.");
                connect(errorWindow, &rdsMailboxWindow::closing, this, [this](QString _){ this->startChecking(); });
            }
            errorWindow->show();
        }
    );

    if (did)
    {
        stopChecking();
    }
}


void rdsMailbox::showMessage(rdsMailboxMessage message)
{
    qDebug()<< message.id;
    stopChecking();
    mailboxWindow = new rdsMailboxWindow();
    mailboxWindow->setMessage(message);
    currentMessage = message;
    connect(mailboxWindow, &rdsMailboxWindow::closing, this, &rdsMailbox::windowClosing);
    mailboxWindow->show();
}

