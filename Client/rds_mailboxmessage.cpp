#include "rds_mailboxmessage.h"

rdsMailboxMessage::rdsMailboxMessage()
{
}


rdsMailboxMessage::rdsMailboxMessage(QString id, QString content)
{
    this->id=id;
    this->content=content;
}


rdsMailboxMessage::rdsMailboxMessage(QJsonObject obj)
{
    id=obj.value("message_id").toString();
    content=obj.value("content").toString();
    color="";
    if (obj.contains("options")) {
        QJsonObject options = obj.value("options").toObject(QJsonObject{});
        color=options.value("color").toString("");
        QVariantList list = options.value("buttons").toArray(QJsonArray{}).toVariantList();
        buttons = QStringList{};
        for (auto& b: list) {
            buttons << b.toString();
        }
    }
}
