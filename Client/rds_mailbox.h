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

class Mailbox : public QObject
{
    Q_OBJECT

public:
    explicit Mailbox(QObject *parent = 0);
    void start();
    QTimer timer;
    MailboxMessage currentMessage;
    rdsMailboxWindow* mailboxWindow;
    void startChecking();
    void stopChecking();

public slots:
    void updateMailbox();
    void windowClosing(QString button);
    void showMessage(MailboxMessage message);

};

#endif // MAILBOX_H
