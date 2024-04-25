#ifndef RDSMAILBOXMESSAGE_H
#define RDSMAILBOXMESSAGE_H

#include <QObject>
#include <QtCore>

class rdsMailboxMessage
{
public:
    QString id;
    QString content;
    QString color;
    QStringList buttons;
    rdsMailboxMessage();
    rdsMailboxMessage(QString id, QString content);
    rdsMailboxMessage(QJsonObject obj);
};

#endif // RDSMAILBOXMESSAGE_H
