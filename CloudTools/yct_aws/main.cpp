
#include <QtCore>
#include <iostream>
#include <stdio.h>

#include <qtaws.h>


int main(int argc, char *argv[])
{
    QSettings settings("credentials.ini", QSettings::IniFormat);

    QString accessKey=settings.value("AWS/AccessKey","").toString();
    QString secretKey=settings.value("AWS/SecretKey","").toString();
    QString region   =settings.value("AWS/Region","us-east-1").toString();

    QCoreApplication app(argc, argv);

    printf("\nYarra CloudTools - AWS Signing Test\n");
    printf("-------------------------------------\n\n");

    QtAWSRequest awsRequest(accessKey, secretKey);

    int ct=0;
    qint64 val=0;

    for (int i=0; i<10; i++)
    {
        qint64 ref=QDateTime::currentMSecsSinceEpoch();

        QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/ping",
                                                QByteArray(), region.toLatin1(), QByteArray(), QStringList());

        qDebug() << i << ": " << QDateTime::currentMSecsSinceEpoch() - ref << " ms";
        val+=QDateTime::currentMSecsSinceEpoch() - ref;
        ct++;

        QJsonDocument jsonReply =QJsonDocument::fromJson(reply.replyData());
        QJsonObject   jsonObject=jsonReply.object();

        QStringList keys = jsonObject.keys();
        foreach(QString key, keys)
        {
            qDebug() << key << ": " << jsonObject[key].toString();
        }
    }

    qDebug() << "Average response: " << val/ct << " ms";

    //reply.printReply();
}
