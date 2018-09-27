#include "yct_api.h"
#include "yct_common.h"
#include "yct_configuration.h"
#include "yct_aws/qtaws.h"

#include "../OfflineReconClient/ort_modelist.h"
#include "../Client/rds_global.h"


yctAPI::yctAPI()
{
    config=0;
    errorReason="";
}


void yctAPI::setConfiguration(yctConfiguration* configuration)
{
    config=configuration;
}


bool yctAPI::validateUser()
{
    QtAWSRequest awsRequest(config->key, config->secret);
    QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/user_status",
                                            QByteArray(), YCT_API_REGION, QByteArray(), QStringList());

    if (!reply.isSuccess())
    {
        //qDebug() << reply.anyErrorString();
        errorReason=reply.anyErrorString();

        if (reply.networkError()==QNetworkReply::ContentOperationNotPermittedError)
        {
            errorReason="YarraCloud Key ID or Secret is invalid.";
        }

        return false;
    }

    //qDebug() << reply.replyData();

    QJsonDocument jsonReply =QJsonDocument::fromJson(reply.replyData());
    QJsonObject   jsonObject=jsonReply.object();
    QStringList   keys      =jsonObject.keys();

    bool canSubmit=false;

    foreach(QString key, keys)
    {
        //qDebug() << key << ": " << jsonObject[key];

        if (key=="can_submit_jobs")
        {
            canSubmit=jsonObject[key].toBool();
            errorReason="Missing payment method or account has been disabled.";
        }
    }

    return canSubmit;
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

    if (!reply.isSuccess())
    {
        errorReason=reply.anyErrorString();
        return -1;
    }
    errorReason="";

    QJsonDocument jsonReply =QJsonDocument::fromJson(reply.replyData());

    int cloudModes=0;

    // TODO: Error handling!

    for (int i=0; i<jsonReply.array().count(); i++)
    {
        QString idName="";
        QString readableName="";
        QString tag="";

        QJsonObject jsonObject=jsonReply.array()[i].toObject();
        QStringList keys = jsonObject.keys();

        foreach(QString key, keys)
        {
            //qDebug() << key << ": " << jsonObject[key].toString();

            if (key=="Name")
            {
                idName=jsonObject[key].toString();
            }

            if (key=="ClientConfig")
            {
                QJsonObject clientConfig     = jsonObject.value(QString("ClientConfig")).toObject();
                QStringList clientConfigKeys = clientConfig.keys();

                foreach(QString clientKey, clientConfigKeys)
                {
                    if (clientKey=="Name")
                    {
                        readableName=clientConfig[clientKey].toString();
                    }

                    if (clientKey=="Tag")
                    {
                        tag=clientConfig[clientKey].toString();
                    }

                    //qDebug() << "ClientConfig: " << clientKey << ": " << clientConfig[clientKey].toString();
                }

                //qDebug() << config;
            }
            // TODO: Properly parse the json structure
        }

        if (!idName.isEmpty())
        {
            if (readableName.isEmpty())
            {
                readableName=idName;
            }

            ortModeEntry* newEntry=new ortModeEntry;
            newEntry->idName=idName;
            newEntry->readableName=readableName;
            newEntry->protocolTag=tag;
            newEntry->computeMode=ortModeEntry::Cloud;
            // Cloud modes always require an ACC because the destination (PACS/drive)
            // can vary among customers
            newEntry->requiresACC=true;
            modeList->modes.append(newEntry);
            cloudModes++;
        }
    }

    return cloudModes;
}


void yctAPI::launchCloudAgent(QString params)
{
    QString cmd=qApp->applicationDirPath() + "/YCA.exe";

    if (!params.isEmpty())
    {
        cmd += " " + params;
    }

    QProcess::startDetached(cmd);
}


bool yctAPI::createCloudFolders()
{
    bool success=true;

    QString appPath=qApp->applicationDirPath()+"/";
    QDir    appDir(qApp->applicationDirPath());

    if (!appDir.mkpath(appPath+YCT_CLOUDFOLDER_IN))
    {
        RTI->log("ERROR: Can't create cloud folder " + QString(appPath+YCT_CLOUDFOLDER_IN));
        success=false;
    }

    if (!appDir.mkpath(appPath+YCT_CLOUDFOLDER_OUT))
    {
        RTI->log("ERROR: Can't create cloud folder " + QString(appPath+YCT_CLOUDFOLDER_OUT));
        success=false;
    }

    if (!appDir.mkpath(appPath+YCT_CLOUDFOLDER_PHI))
    {
        RTI->log("ERROR: Can't create cloud folder " + QString(appPath+YCT_CLOUDFOLDER_PHI));
        success=false;
    }

    if (!appDir.mkpath(appPath+YCT_CLOUDFOLDER_ARCHIVE))
    {
        RTI->log("ERROR: Can't create cloud folder " + QString(appPath+YCT_CLOUDFOLDER_ARCHIVE));
        success=false;
    }

    return success;
}


QString yctAPI::getCloudPath(QString folder)
{
    return qApp->applicationDirPath()+"/"+folder;
}


QString yctAPI::createUUID()
{
    QString uuidString=QUuid::createUuid().toString();
    // Remove the curly braces enclosing the id
    uuidString=uuidString.mid(1,uuidString.length()-2);
    return uuidString;
}

