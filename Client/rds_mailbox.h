#ifndef MAILBOX_H
#define MAILBOX_H

#include <QObject>
#include <QtCore>
#include <QMessageBox>
#include "rds_global.h"
#include "rds_mailboxwindow.h"
#include <../NetLogger/netlogger.h>

class MailboxMessage
{
public:
    QString id;
    QString content;
    MailboxMessage();
    MailboxMessage(QString id, QString content);
    MailboxMessage(QJsonObject obj);
};

class MailboxWorker : public QObject
{
    Q_OBJECT

    NetLogger netLogger;
    bool waiting;
    QNetworkReply* currentReply;
public:
    explicit MailboxWorker(QObject *parent = 0);
    void start();
    QTimer timer;
    QTimer timeout;
    MailboxMessage currentMessage;
    rdsMailboxWindow* mailboxWindow;

signals:
    void newMessage(MailboxMessage message);
public slots:
    void updateMailbox();
    void onMarkedMessageResponse();
    void onNewMessagesResponse();
    void replyTimeout();
    void windowClosing(QString button);
    void showMessage(MailboxMessage message);

};


class Mailbox: public QObject
{
    Q_OBJECT
//    QThread mailboxThread;
//    QTimer mailboxTimer;
    MailboxWorker worker;
public:
    explicit Mailbox(QObject *parent = 0);
};


#endif // MAILBOX_H
