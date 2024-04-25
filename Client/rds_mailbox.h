#ifndef MAILBOX_H
#define MAILBOX_H

#include <QObject>
#include <QtCore>
#include <QMessageBox>
#include "rds_global.h"
#include "rds_mailboxwindow.h"
#include "rds_mailboxmessage.h"
#include <../NetLogger/netlogger.h>
#include <functional>

class rdsMailbox : public QObject
{
    Q_OBJECT

public:
    explicit rdsMailbox(QObject *parent = 0);
    void start();
    QTimer timer;
    rdsMailboxMessage currentMessage;
    rdsMailboxWindow* mailboxWindow;
    void startChecking();
    void stopChecking();
    unsigned int errorCount = 0;
    int checkInterval = 1000;
public slots:
    void updateMailbox();
    void windowClosing(QString button);
    void showMessage(rdsMailboxMessage message);

};


#endif // MAILBOX_H
