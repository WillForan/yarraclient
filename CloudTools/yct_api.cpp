#include "yct_api.h"
#include "yct_common.h"
#include "yct_configuration.h"
#include "yct_aws/qtaws.h"
#include "../CloudAgent/yca_task.h"

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


#ifdef YARRA_APP_SAC

#define keycmp(a,b) QString::compare(a,b,Qt::CaseInsensitive)==0

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
        QJsonObject jsonObject=jsonReply.array()[i].toObject();
        QStringList keys = jsonObject.keys();

        ortModeEntry* newEntry=new ortModeEntry;

        foreach(QString key, keys)
        {
            //qDebug() << key << ": " << jsonObject[key].toString();

            if (key=="Name")
            {
                newEntry->idName=jsonObject[key].toString();
            }

            if (key=="ClientConfig")
            {
                QJsonObject clientConfig     = jsonObject.value(QString("ClientConfig")).toObject();
                QStringList clientConfigKeys = clientConfig.keys();

                foreach(QString clientKey, clientConfigKeys)
                {
                    if (keycmp(clientKey,"Name"))
                    {
                        newEntry->readableName=clientConfig[clientKey].toString();
                    }

                    if (keycmp(clientKey,"Tag"))
                    {
                        newEntry->protocolTag=clientConfig[clientKey].toString();
                    }

                    if (keycmp(clientKey,"MinimumSizeMB"))
                    {
                        newEntry->minimumSizeMB=clientConfig[clientKey].toDouble();
                    }

                    if (keycmp(clientKey,"ParamLabel"))
                    {
                        newEntry->paramLabel=clientConfig[clientKey].toString();
                    }

                    if (keycmp(clientKey,"ParamDescription"))
                    {
                        newEntry->paramDescription=clientConfig[clientKey].toString();
                    }

                    if (keycmp(clientKey,"ParamDefault"))
                    {
                        newEntry->paramDefault=clientConfig[clientKey].toDouble();
                    }

                    if (keycmp(clientKey,"ParamMin"))
                    {
                        newEntry->paramMin=clientConfig[clientKey].toDouble();
                    }

                    if (keycmp(clientKey,"ParamMax"))
                    {
                        newEntry->paramMax=clientConfig[clientKey].toDouble();
                    }

                    if (keycmp(clientKey,"ParamIsFloat"))
                    {
                        newEntry->paramIsFloat=clientConfig[clientKey].toBool();
                    }

                    //qDebug() << "ClientConfig: " << clientKey << ": " << clientConfig[clientKey].toString();
                }

                //qDebug() << config;
            }
        }

        if (!newEntry->idName.isEmpty())
        {
            if (newEntry->readableName.isEmpty())
            {
                newEntry->readableName=newEntry->idName;
            }

            // If the parameter is an integer parameter, then round all values to integers
            if (!newEntry->paramIsFloat)
            {
                newEntry->paramDefault=int(newEntry->paramDefault);
                newEntry->paramMin    =int(newEntry->paramMin);
                newEntry->paramMax    =int(newEntry->paramMax);
            }

            // Cloud modes always require an ACC because the destination (PACS/drive)
            // can vary among customers
            newEntry->requiresACC=true;
            newEntry->computeMode=ortModeEntry::Cloud;

            modeList->modes.append(newEntry);
            cloudModes++;
        }
        else
        {
            delete newEntry;
            newEntry=0;
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

#endif


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


bool yctAPI::uploadCase(ycaTask* task)
{
    return true;
}

