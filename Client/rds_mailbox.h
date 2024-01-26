#ifndef MAILBOX_H
#define MAILBOX_H

#include <QObject>
#include <QtCore>
#include <QMessageBox>
#include "rds_global.h"
#include "rds_mailboxwindow.h"
#include <../NetLogger/netlogger.h>
#include <functional>
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
    void start();
    QTimer timer;
    MailboxMessage currentMessage;
    rdsMailboxWindow* mailboxWindow;
    void startChecking();
    void stopChecking();

    template <typename F>
    bool doRequest(QString endpoint, F&& fn);
public slots:
    void updateMailbox();
//    void onMarkedMessageResponse(QNetworkReply* reply);
//    void onNewMessagesResponse(QNetworkReply* reply);
//    void replyTimeout(QNetworkReply*);
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
