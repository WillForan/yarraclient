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

public:
    explicit MailboxWorker(QObject *parent = 0);
    QTimer timer;
    MailboxMessage currentMessage;

signals:
    void newMessage(MailboxMessage message);
public slots:
    void onTimeout();
    void onMarkedMessageResponse(QNetworkReply* reply);
    void onNewMessagesResponse(QNetworkReply* reply);
    void windowClosing(QString button);
};


class Mailbox: public QObject
{
    Q_OBJECT
    QThread mailboxThread;
    QTimer mailboxTimer;
    MailboxWorker worker;
    rdsMailboxWindow* mailboxWindow;
public:
    explicit Mailbox(QObject *parent = 0);
public slots:
    void showMessage(MailboxMessage message);

};


#endif // MAILBOX_H
