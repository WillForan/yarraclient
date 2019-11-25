#include "yct_api.h"
#include "yct_configuration.h"
#include "yct_aws/qtaws.h"
#include "../CloudAgent/yca_threadlog.h"
#include "../CloudAgent/yca_global.h"
#include "../CloudAgent/yca_task.h"

#include "../OfflineReconClient/ort_modelist.h"
#include "../Client/rds_global.h"


#if defined(Q_OS_WIN)
    #include <QtCore/QLibrary>
    #include <QtCore/qt_windows.h>
#endif
#if defined(Q_OS_UNIX)
    #include <time.h>
#endif

#include <QEventLoop>
#include <QTimer>
#include <QProcess>


yctTransferInformation::yctTransferInformation()
{
    username="";
    inBucket="";
    outBucket="";
    region="";
    userAllowed=false;    
}


yctStorageInformation::yctStorageInformation()
{
    method=ystInvalid;

    pacsIP="";
    pacsPort="";
    pacsAET="Yarra";
    pacsAEC="Yarra";

    driveLocation="";
}


yctAPI::yctAPI()
{
    config=0;
    errorReason="";

    loadCertificate();
}


void yctAPI::setConfiguration(yctConfiguration* configuration)
{
    config=configuration;
}


bool yctAPI::validateUser(yctTransferInformation* transferInformation)
{
    QtAWSRequest awsRequest(config->key, config->secret);
    QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/user_status",
                                            QByteArray(), YCT_API_REGION, QByteArray(), QStringList());

    if (!reply.isSuccess())
    {
        errorReason=reply.anyErrorString();

        if (reply.networkError()==QNetworkReply::ContentOperationNotPermittedError)
        {
            errorReason="YarraCloud Key ID or Secret is invalid.";
        }

        YTL->log("AWS response error. Reason: "+errorReason,YTL_ERROR,YTL_MID);
        return false;
    }

    //qInfo() << reply.replyData();

    QJsonDocument jsonReply =QJsonDocument::fromJson(reply.replyData());
    QJsonObject   jsonObject=jsonReply.object();
    QStringList   keys      =jsonObject.keys();

    bool canSubmit=false;

    foreach(QString key, keys)
    {
        if (key=="can_submit_jobs")
        {
            canSubmit=jsonObject[key].toBool();
            errorReason="Missing payment method or account has been disabled.";

            if (!canSubmit)
            {
                YTL->log(errorReason,YTL_ERROR,YTL_MID);
            }
        }

        if (transferInformation!=0)
        {
            if (key=="username")
            {
                transferInformation->username=jsonObject[key].toString();
            }
            if (key=="in_bucket")
            {
                transferInformation->inBucket=jsonObject[key].toString();
            }
            if (key=="out_bucket")
            {
                transferInformation->outBucket=jsonObject[key].toString();                
            }
            if (key=="region")
            {
                transferInformation->region=jsonObject[key].toString();
            }

            transferInformation->userAllowed=canSubmit;
        }

        //qInfo() << "in_bucket " << transferInformation->inBucket;
        //qInfo() << "region " << transferInformation->region;
    }

    return canSubmit;
}


#if defined(YARRA_APP_SAC) || defined(YARRA_APP_ORT)

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

    QJsonDocument jsonReply=QJsonDocument::fromJson(reply.replyData());

    int cloudModes=0;

    // TODO: Error handling!

    for (int i=0; i<jsonReply.array().count(); i++)
    {
        QJsonObject jsonObject=jsonReply.array()[i].toObject();
        QStringList keys=jsonObject.keys();

        ortModeEntry* newEntry=new ortModeEntry();

        // Requestion additional files is not supported for cloud recons at the moment,
        // because we cannot guarantee that these files do not contain PHI (if non-Twix
        // files are selected)
        newEntry->requestAdditionalFiles=false;

        foreach(QString key, keys)
        {
            qDebug() << key << ": " << jsonObject[key].toString();

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
                        newEntry->minimumSizeMB=clientConfig[clientKey].toString().toDouble();
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
                        newEntry->paramDefault=clientConfig[clientKey].toString().toDouble();
                    }

                    if (keycmp(clientKey,"ParamMin"))
                    {
                        newEntry->paramMin=clientConfig[clientKey].toString().toDouble();
                    }

                    if (keycmp(clientKey,"ParamMax"))
                    {
                        newEntry->paramMax=clientConfig[clientKey].toString().toDouble();
                    }

                    if (keycmp(clientKey,"ParamIsFloat"))
                    {
                        newEntry->paramIsFloat=(clientConfig[clientKey].toString()=="TRUE");
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

    modeList->count=modeList->modes.count();

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
    return qApp->applicationDirPath()+folder;
}


QString yctAPI::createUUID()
{
    QString uuidString=QUuid::createUuid().toString();
    // Remove the curly braces enclosing the id
    uuidString=uuidString.mid(1,uuidString.length()-2);
    return uuidString;
}


bool yctAPI::getJobStatus(ycaTaskList* taskList)
{
    if (taskList->empty())
    {
        return true;
    }

    QString runningTasks="[";
    for (int i=0; i<taskList->count(); i++)
    {
        runningTasks+=" \"" + taskList->at(i)->uuid + "\" ";

        if (i<taskList->count()-1)
        {
            runningTasks+=", ";
        }

        // Make sure to flag the job status as invalid in case the job can't
        // be found in the cloud database
        taskList->at(i)->status=ycaTask::tsInvalid;
    }
    runningTasks+="]";

    QByteArray content=runningTasks.toUtf8();

    QtAWSRequest awsRequest(config->key, config->secret);
    QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/jobs",
                                            QByteArray(), YCT_API_REGION, content, QStringList());

    if (!reply.isSuccess())
    {
        qDebug() << reply.anyErrorString();
        errorReason=reply.anyErrorString();

        if (reply.networkError()==QNetworkReply::ContentOperationNotPermittedError)
        {
            errorReason="YarraCloud Key ID or Secret is invalid.";
        }

        YTL->log("AWS response error. Reason: "+errorReason,YTL_ERROR,YTL_MID);
        return false;
    }

    //qDebug() << "Response:" << reply.replyData();

    QJsonDocument   jsonReply =QJsonDocument::fromJson(reply.replyData());
    QJsonObject     jsonObject=jsonReply.object();

    for (int i=0; i<taskList->count(); i++)
    {
        QJsonObject taskInfo      =jsonObject[taskList->at(i)->uuid].toObject();
        QJsonObject batchInfo     =taskInfo["batch_info"].toObject();
        QString     batchStatusStr=batchInfo["status"].toString();

        if (batchStatusStr.isEmpty())
        {
            continue;
        }

        yctAWSCommon::BatchStatus batchStatus=yctAWSCommon::getBatchStatus(batchStatusStr);

        switch (batchStatus)
        {
        case yctAWSCommon::SUBMITTED:
        case yctAWSCommon::PENDING:
        case yctAWSCommon::RUNNABLE:
        case yctAWSCommon::STARTING:
        case yctAWSCommon::RUNNING:
            taskList->at(i)->status=ycaTask::tsRunning;
            break;

        case yctAWSCommon::SUCCEEDED:
            taskList->at(i)->status=ycaTask::tsReady;
            break;

        case yctAWSCommon::FAILED:
            taskList->at(i)->status=ycaTask::tsErrorProcessing;
            break;

        case yctAWSCommon::INVALID:
        default:
            // TODO: Error reporting
            return false;
            break;
        }

        if (!taskInfo["cost"].isUndefined())
        {
            taskList->at(i)->cost=taskInfo["cost"].toDouble();
        }

        if (!taskInfo["id"].isUndefined())
        {
            taskList->at(i)->shortcode=taskInfo["id"].toString();
        }
    }

    return true;
}


bool yctAPI::uploadCase(ycaTask* task, yctTransferInformation* setup, QMutex* mutex)
{
    bool success=false;

    YTL->log("Uploading case "+task->uuid,YTL_INFO,YTL_HIGH);

    // Compose the command line for the help app
    QString cmdLine=setup->username + " " + config->key + " " + config->secret +" " +
                    setup->region + " " + setup->inBucket + " " + task->uuid + " upload ";

    QString path=getCloudPath(YCT_CLOUDFOLDER_OUT)+"/";

    for (int i=0; i<task->twixFilenames.count(); i++)
    {
        YTL->log("Including file "+task->twixFilenames.at(i),YTL_INFO,YTL_LOW);
        cmdLine += path+task->twixFilenames.at(i)+" ";
    }

    cmdLine += path+task->taskFilename;    
    qInfo() << cmdLine;

    // TODO: Batch call if commandline is too long

    // Execute the helper app to perform the multi-part upload
    YTL->log("Calling upload helper",YTL_INFO,YTL_LOW);
    int exitcode=callHelperApp(YCT_HELPER_APP,cmdLine,YCT_HELPER_TIMEOUT_TRANSFER);
    YTL->log("Upload helper finished",YTL_INFO,YTL_LOW);

    YTL->log("(Helper output begin)",YTL_INFO,YTL_LOW);
    for (int i=0; i<helperAppOutput.count(); i++)
    {
        YTL->log(helperAppOutput.at(i),YTL_INFO,YTL_LOW);
    }
    YTL->log("(Helper output end)",YTL_INFO,YTL_LOW);

    if (exitcode==0)
    {
        success=true;
    }
    else
    {
        YTL->log("Upload helper reported error",YTL_ERROR,YTL_MID);
        success=false;
        // TODO: Error handling        
    }

    if (success)
    {
        // Call the job submission endpoint to trigger the processing
        YTL->log("Calling API to trigger job",YTL_INFO,YTL_LOW);

        QString      jsonText="{\"name\":\""+ task->uuid +"\", \"task_file_name\": \"" + task->taskFilename +"\", \"recon_mode\":\"" + task->reconMode + "\"}";
        QByteArray   jsonString=jsonText.toUtf8();
        QtAWSRequest awsRequest(config->key, config->secret);
        QtAWSReply   reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/job_submit",
                                                  QByteArray(), YCT_API_REGION, jsonString, QStringList());

        if (!reply.isSuccess())
        {
            errorReason=reply.anyErrorString();

            if (reply.networkError()==QNetworkReply::ContentOperationNotPermittedError)
            {
                errorReason="YarraCloud Key ID or Secret is invalid.";
            }

            YTL->log("AWS response error. Reason: "+errorReason,YTL_ERROR,YTL_MID);

            // TODO: Error handling
            success=false;
        }

        // qDebug() << reply.replyData();
        // TODO: Evaluate returned json
    }

    // If successful, delete TWIX and task file from OUT folder    
    YTL->log("Removing TWIX and task files",YTL_INFO,YTL_LOW);

    if (success)
    {
        mutex->lock();

        for (int i=0; i<task->twixFilenames.count(); i++)
        {
            if (!QFile::remove(path+task->twixFilenames.at(i)))
            {
                YTL->log("Unable to remove TWIX file: "+task->twixFilenames.at(i),YTL_ERROR,YTL_MID);
                // TODO: Error handling
            }
        }

        if (!QFile::remove(path+task->taskFilename))
        {
            YTL->log("Unable to remove task file: "+task->taskFilename,YTL_ERROR,YTL_MID);
            // TODO: Error handling
        }

        mutex->unlock();
    }

    return success;
}


bool yctAPI::downloadCase(ycaTask* task, yctTransferInformation* setup, QMutex* mutex)
{
    bool success=false;
    YTL->log("Downloading case "+task->uuid,YTL_INFO,YTL_HIGH);

    QString path=getCloudPath(YCT_CLOUDFOLDER_IN)+"/"+task->uuid;

    QString cmdLine=setup->username + " " + config->key + " " + config->secret +" " +
                    setup->region + " " + setup->outBucket + " " + task->uuid + " download " + getCloudPath(YCT_CLOUDFOLDER_IN);
    //qDebug() << cmdLine;

    QDir dir(getCloudPath(YCT_CLOUDFOLDER_IN));
    if (!dir.mkpath(path))
    {
        YTL->log("Unable to create download folder "+path,YTL_ERROR,YTL_MID);
        // TODO: Error reporting
        return false;
    }

    // Create INCOMPLETE file for detecting interrupted downloads
    QFile incompleteFile(path+"/"+YCT_INCOMPLETE_FILE);
    incompleteFile.open(QIODevice::ReadWrite);
    QString incompleteContent="INCOMPLETE YARRACLOUD DOWNLOAD " + QDateTime::currentDateTime().toString();
    incompleteFile.write(incompleteContent.toUtf8());
    incompleteFile.close();

    // TODO: Check available disk space

    YTL->log("Calling download helper",YTL_INFO,YTL_LOW);
    int exitcode=callHelperApp(YCT_HELPER_APP,cmdLine,YCT_HELPER_TIMEOUT_TRANSFER);
    YTL->log("Download helper finished",YTL_INFO,YTL_LOW);

    YTL->log("(Helper output begin)",YTL_INFO,YTL_LOW);
    for (int i=0; i<helperAppOutput.count(); i++)
    {
        YTL->log(helperAppOutput.at(i),YTL_INFO,YTL_LOW);
    }
    YTL->log("(Helper output end)",YTL_INFO,YTL_LOW);

    if (exitcode==0)
    {
        success=true;

        // Remove INCOMPLETE file
        if (!QFile::remove(path+"/"+YCT_INCOMPLETE_FILE))
        {
            // TODO: Error handling
            YTL->log("Unable to remove INCOMPLETE file in "+path,YTL_ERROR,YTL_MID);
        }
    }
    else
    {
        YTL->log("Exec error. Return value: "+QString::number(exitcode),YTL_ERROR,YTL_MID);
        success=false;
        // TODO: Error handling
    }

    return success;
}


bool yctAPI::insertPHI(QString path, ycaTask* task)
{
    QDir tarDir(path+"/output_files");

    if (!tarDir.exists())
    {
        // TODO: Error handling
        YTL->log("Folder with extracted files not found: "+tarDir.absolutePath(),YTL_ERROR,YTL_MID);
        return false;
    }

    if (tarDir.entryInfoList(QStringList("*"),QDir::Files).isEmpty())
    {
        // TODO: Error handling
        YTL->log("Reconstruction created no output",YTL_ERROR,YTL_MID);
        return false;
    }

    QString dicomPath=QDir::toNativeSeparators(tarDir.absolutePath()+"/*.dcm")+" ";

    QString cmd="";
    cmd += "-i \"(0010,0010)="+task->patientName+"\" ";
    cmd += "-i \"(0010,0020)="+task->mrn+"\" ";
    cmd += "-i \"(0010,0030)="+task->dob+"\" ";
    cmd += "-i \"(0008,0050)="+task->acc+"\" ";
    cmd += "-i \"(0010,4000)="+task->uuid+"\" ";
    cmd += "-nb ";
    cmd += dicomPath;

    //qDebug() << "CMD: " << cmd;

    bool helperSuccess=callHelperApp(YCT_DCMTK_DCMODIFY,cmd)==0;

    YTL->log("(Helper output begin)",YTL_INFO,YTL_LOW);
    for (int i=0; i<helperAppOutput.count(); i++)
    {
        YTL->log(helperAppOutput.at(i),YTL_INFO,YTL_LOW);
    }
    YTL->log("(Helper output end)",YTL_INFO,YTL_LOW);

    if (!helperSuccess)
    {
        YTL->log("Error executing dcmodify",YTL_ERROR,YTL_MID);
        return false;
    }

    // TODO: Search helper output for error messages

    return true;
}


int yctAPI::callHelperApp(QString binary, QString parameters, int execTimeout)
{
    // Clear the output buffer
    helperAppOutput.clear();
    int exitcode=-1;
    bool success=false;

    QProcess *myProcess = new QProcess(0);
    myProcess->setReadChannel(QProcess::StandardOutput);
    myProcess->setProcessChannelMode(QProcess::MergedChannels);
    QString helperCmd=qApp->applicationDirPath()+"/"+binary+" "+parameters;

    // If a proxy has been configured for YarraCloud, set environment variables
    // so that the Go-based uploader will route the traffic
    if (!config->proxyIP.isEmpty())
    {
        QString proxyString=config->proxyIP+":"+QString::number(config->proxyPort);

        // Add authentication information if required by the proxy server
        if (!config->proxyUsername.isEmpty())
        {
            proxyString=config->proxyUsername+":"+config->proxyPassword+"@"+proxyString;
        }
        proxyString="http://"+proxyString;

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("HTTP_PROXY",  proxyString);
        env.insert("HTTPS_PROXY", proxyString);
        myProcess->setProcessEnvironment(env);
        YTL->log("Using Proxy "+proxyString,YTL_INFO,YTL_LOW);
    }

    //qDebug() << "Calling helper tool: " + helperCmd;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(execTimeout);
    QEventLoop q;
    connect(myProcess, SIGNAL(finished(int , QProcess::ExitStatus)), &q, SLOT(quit()));
    connect(&timeoutTimer, SIGNAL(timeout()), &q, SLOT(quit()));

    QTime ti;
    ti.start();
    timeoutTimer.start();
    myProcess->start(helperCmd);
    q.exec();

    // Check for problems with the event loop: Sometimes it seems to return to quickly!
    // In this case, start a second while loop to check when the process is really finished.
    if ((timeoutTimer.isActive()) && (myProcess->state()==QProcess::Running))
    {
        timeoutTimer.stop();
        RTI->log("Warning: QEventLoop returned too early. Starting secondary loop.");
        YTL->log("Warning: QEventLoop returned too early. Starting secondary loop.",YTL_WARNING,YTL_MID);

        while ((myProcess->state()==QProcess::Running) && (ti.elapsed()<execTimeout))
        {
            RTI->processEvents();
            Sleep(RDS_SLEEP_INTERVAL);
        }

        // If the process did not finish within the timeout duration
        if (myProcess->state()==QProcess::Running)
        {
            RTI->log("Warning: Process is still active. Killing process.");
            YTL->log("Warning: Process is still active. Killing process.",YTL_WARNING,YTL_MID);

            myProcess->kill();
            success=false;
        }
        else
        {
            success=true;
        }
    }
    else
    {
        // Normal timeout-handling if QEventLoop works normally
        if (timeoutTimer.isActive())
        {
            success=true;
            timeoutTimer.stop();
        }
        else
        {
            RTI->log("Warning: Process event loop timed out.");
            YTL->log("Warning: Process event loop timed out.",YTL_WARNING,YTL_MID);

            RTI->log("Warning: Duration since start "+QString::number(ti.elapsed())+" ms");
            YTL->log("Warning: Duration since start "+QString::number(ti.elapsed())+" ms",YTL_WARNING,YTL_MID);

            success=false;
            if (myProcess->state()==QProcess::Running)
            {
                RTI->log("Warning: Process is still active. Killing process.");
                YTL->log("Warning: Process is still active. Killing process.",YTL_WARNING,YTL_MID);

                myProcess->kill();
            }
        }
    }

    if (!success)
    {
        // The process timeed out. Probably some error occured.
        RTI->log("ERROR: Timeout during call of YCA helper!");
        YTL->log("ERROR: Timeout during call of YCA helper!",YTL_ERROR,YTL_MID);

        exitcode=-1;
    }
    else
    {
        // Read the output lines from the process and store in string list
        // for parsing
        char buf[1024];
        qint64 lineLength = -1;

        do
        {
            lineLength=myProcess->readLine(buf, sizeof(buf));
            if (lineLength != -1)
            {
                helperAppOutput << QString(buf).trimmed();
            }
        } while (lineLength!=-1);

        if (myProcess->exitStatus()==QProcess::NormalExit)
        {
            exitcode=myProcess->exitCode();
        }
    }

    delete myProcess;
    myProcess=0;

    YTL->log("Process exitcode: "+QString::number(exitcode),YTL_INFO,YTL_LOW);
    return exitcode;
}


bool yctAPI::pushToDestinations(QString path, ycaTask* task)
{
    // Query the destinations of the mode
    QString reqString="{ \"recon_mode\": \"" + task->reconMode + "\" }";
    QByteArray content=reqString.toUtf8();

    QtAWSRequest awsRequest(config->key, config->secret);
    QtAWSReply reply=awsRequest.sendRequest("POST", "api.yarracloud.com", "v1/mode_destinations",
                                            QByteArray(), YCT_API_REGION, content, QStringList());
    if (!reply.isSuccess())
    {
        // TODO: Error handling
        errorReason=reply.anyErrorString();

        if (reply.networkError()==QNetworkReply::ContentOperationNotPermittedError)
        {
            errorReason="YarraCloud Key ID or Secret is invalid.";
        }

        YTL->log("AWS response error. Reason: "+errorReason,YTL_ERROR,YTL_MID);
        return false;
    }

    yctStorageList storageList;
    QJsonDocument  jsonReply=QJsonDocument::fromJson(reply.replyData());

    for (int i=0; i<jsonReply.array().count(); i++)
    {
        QJsonObject jsonObject=jsonReply.array()[i].toObject();

        yctStorageInformation* destination=new yctStorageInformation();
        storageList.append(destination);

        if (jsonObject["method"].toString()=="smb")
        {
            destination->method=yctStorageInformation::yctDrive;
            destination->driveLocation=jsonObject["location"].toString();
        }
        else
        {
            if (jsonObject["method"].toString()=="PACS")
            {
                destination->method  =yctStorageInformation::yctPACS;
                destination->pacsIP  =jsonObject["ip"].toString();
                destination->pacsPort=jsonObject["port"].toString();
                destination->pacsAET =jsonObject["aet"].toString();
                destination->pacsAEC =jsonObject["aec"].toString();
            }
            else
            {
                YTL->log("Unknown storage destination: "+jsonObject["method"].toString(),YTL_WARNING,YTL_HIGH);
                YTL->log("Destination name: "+jsonObject["name"].toString(),YTL_WARNING,YTL_HIGH);
                delete destination;
                destination=0;
                continue;
            }
        }

        destination->name=jsonObject["name"].toString();
    }

    int failedTransfers=0;

    while (!storageList.isEmpty())
    {
        yctStorageInformation* destination=storageList.takeFirst();

        if (destination->method==yctStorageInformation::yctDrive)
        {
            if (!pushToDrive(path, task, destination))
            {
                // TODO: Error handling
                YTL->log("Drive transfer failed to "+ destination->name,YTL_ERROR,YTL_HIGH);
                failedTransfers++;
            }
        }

        if (destination->method==yctStorageInformation::yctPACS)
        {
            if (!pushToPACS(path, task, destination))
            {
                // TODO: Error handling
                YTL->log("PACS transfer failed to "+ destination->name,YTL_ERROR,YTL_HIGH);
                failedTransfers++;
            }
        }

        delete destination;
        destination=0;
    }

    if (failedTransfers>0)
    {
        YTL->log("Failed storage transfers: "+QString::number(failedTransfers),YTL_WARNING,YTL_HIGH);
    }

    return (failedTransfers==0);
}


bool yctAPI::pushToPACS (QString path, ycaTask* task, yctStorageInformation* destination)
{
    qDebug() << "Pushing to PACS destination " << destination->name;

    QDir dcmDir(path+"/output_files");

    if (!dcmDir.exists())
    {
        // TODO: Error handling
        YTL->log("Folder with extracted files does not exist:  "+dcmDir.absolutePath(),YTL_ERROR,YTL_MID);
        return false;
    }

    if (dcmDir.entryInfoList(QStringList("*"),QDir::Files).isEmpty())
    {
        // TODO: Error handling
        YTL->log("Reconstruction created no output",YTL_ERROR,YTL_MID);
        return false;
    }

    // TODO: Possibly change to file-by-file sending to ensure dicoms are sent in order

    QString dicomPath=QDir::toNativeSeparators(dcmDir.absolutePath()+"/*.dcm")+" ";

    QString cmd="--timeout 20 +sd ";
    cmd += "-aec "+destination->pacsAEC+" ";
    cmd += "-aet "+destination->pacsAET+" ";
    cmd += destination->pacsIP+" ";
    cmd += destination->pacsPort+" ";
    cmd += dicomPath;

    //qDebug() << "CMD: " << cmd;

    bool success=true;

    if (callHelperApp(YCT_DCMTK_STORESCU,cmd)!=0)
    {
        // TODO: Error handling
        YTL->log("Error executing storescu",YTL_ERROR,YTL_MID);
        success=false;
    }

    // TODO: Search helper output for error messages

    YTL->log("(Helper output begin)",YTL_INFO,YTL_LOW);
    for (int i=0; i<helperAppOutput.count(); i++)
    {
        YTL->log(helperAppOutput.at(i),YTL_INFO,YTL_LOW);
    }
    YTL->log("(Helper output end)",YTL_INFO,YTL_LOW);

    return success;
}


bool yctAPI::pushToDrive(QString path, ycaTask* task, yctStorageInformation* destination)
{
    QDir sourceDir(path+"/output_files");
    if (!sourceDir.exists())
    {
        YTL->log("Can't find reconstructed files",YTL_ERROR,YTL_MID);
        return false;
    }

    QString fullPath=destination->driveLocation+"/"+task->taskID;

    QDir dir;
    if (dir.exists(fullPath))
    {
        QString timeStamp=QDateTime::currentDateTime().toString("ddMMyyHHmmsszzz");
        fullPath=fullPath+"_"+timeStamp;

        if (dir.exists(fullPath))
        {
            YTL->log("Can't create unique folder for TaskID",YTL_ERROR,YTL_MID);
            return false;
        }
    }

    if (!dir.mkpath(fullPath))
    {
        YTL->log("Can't create destination path: "+fullPath,YTL_ERROR,YTL_MID);
        // TODO: Error handling
        return false;
    }

    // TODO: Check for disk space
    // TODO: Recursively copy also folders
    QStringList files=sourceDir.entryList(QStringList("*"),QDir::Files,QDir::Name);

    for (int i=0; i<files.count(); i++)
    {
        if (!QFile::copy(sourceDir.absoluteFilePath(files.at(i)),fullPath+"/"+files.at(i)))
        {
            YTL->log("Error copying file: "+files.at(i),YTL_ERROR,YTL_MID);
            return false;
        }
    }

    return true;
}


bool yctAPI::loadCertificate()
{
    // Read certificate from external file
    QFile certificateFile(qApp->applicationDirPath() + "/yarracloud.crt");

    // Read the certificate if possible and then add it
    if (certificateFile.open(QIODevice::ReadOnly))
    {
        const QByteArray certificateContent=certificateFile.readAll();

        // Create a certificate object
        const QSslCertificate certificate(certificateContent);

        // Add this certificate to all SSL connections
        QSslSocket::addDefaultCaCertificate(certificate);

        qInfo() << "Loaded yarracloud certificate from file.";
    }

    return true;
}

