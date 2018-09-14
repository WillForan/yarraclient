#include "yct_api.h"
#include "yct_common.h"
#include "yct_configuration.h"
#include "yct_aws/qtaws.h"

#include "../OfflineReconClient/ort_modelist.h"



yctAPI::yctAPI()
{
    config=0;
}


void yctAPI::setConfiguration(yctConfiguration* configuration)
{
    config=configuration;
}


int yctAPI::readModeList(ortModeList* modeList)
{
    if ((!config) || (config->key.isEmpty()) || (config->secret.isEmpty()))
    {
        return 0;
    }

    QtAWSRequest awsRequest(config->key, config->secret);
    QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/modes",
                                            QByteArray(), YCT_API_REGION, QByteArray(), QStringList());

    QJsonDocument jsonReply =QJsonDocument::fromJson(reply.replyData());

    int cloudModes=0;

    // TODO: Error handling!

    for (int i=0; i<jsonReply.array().count(); i++)
    {
        QString idName="";

        QJsonObject jsonObject=jsonReply.array()[i].toObject();
        QStringList keys = jsonObject.keys();

        foreach(QString key, keys)
        {
            qDebug() << key << ": " << jsonObject[key].toString();

            if (key=="Name")
            {
                idName=jsonObject[key].toString();
            }

            if (key=="ClientConfig")
            {
                QJsonObject config = jsonObject.value(QString("ClientConfig")).toObject();
                qDebug() << config;
            }
            // TODO: Properly parse the json structure
        }

        if (!idName.isEmpty())
        {
            ortModeEntry* newEntry=new ortModeEntry;
            newEntry->idName=idName;
            newEntry->readableName=idName;
            newEntry->computeMode=ortModeEntry::Cloud;
            modeList->modes.append(newEntry);
            cloudModes++;
        }
    }

    return cloudModes;
}


void yctAPI::launchCloudAgent()
{
    QProcess::startDetached(qApp->applicationDirPath() + "/YCA.exe");
}



