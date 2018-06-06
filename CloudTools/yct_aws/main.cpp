
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

    for (int i=0; i<1; i++)
    {
        qint64 ref=QDateTime::currentMSecsSinceEpoch();

        QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/modes",
                                                QByteArray(), region.toLatin1(), QByteArray(), QStringList());

        val+=QDateTime::currentMSecsSinceEpoch() - ref;
        ct++;
        qDebug() << i << ": " << val << " ms";
        qDebug() << "";

        /*
        qDebug() << reply.anyErrorString();
        qDebug() << reply.awsErrorString();
        qDebug() << reply.networkErrorString();
        qDebug() << reply.networkError();
        qDebug() << reply.replyData();
        reply.printReply();
        */

        QJsonDocument jsonReply =QJsonDocument::fromJson(reply.replyData());

        for (int i=0; i<jsonReply.array().count(); i++)
        {
            qDebug() << "-- Mode " << i << "--";

            QJsonObject jsonObject=jsonReply.array()[i].toObject();

            QStringList keys = jsonObject.keys();
            foreach(QString key, keys)
            {
                qDebug() << key << ": " << jsonObject[key].toString();
            }

            qDebug() << "";
        }
    }

    qDebug() << "Average response: " << val/ct << " ms";

    //reply.printReply();
}
