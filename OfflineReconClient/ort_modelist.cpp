#include <QtWidgets>

#include "ort_modelist.h"
#include "ort_global.h"



#ifdef YARRA_APP_ORT
    #include "ort_network.h"
#endif
#ifdef YARRA_APP_SAC
    #include "sac_network.h"
#endif



ortModeList::ortModeList()
{
    network=0;
    modes.clear();
    count=0;
}

ortModeList::~ortModeList()
{
    // Clean up the mode list
    while (!modes.isEmpty())
    {
        delete modes.takeFirst();
    }
}



bool ortModeList::readModeList()
{
    bool error=false;
    QString errorText="";

    if (network==0)
    {
        RTI->log("ERROR: Network instance has not been set.");
        return false;
    }

    // Check if the mode file exists and if it's readable
    QString modeFileName=network->serverPath+"/"+ORT_MODEFILE;
    QString serverFileName=network->serverPath+"/"+ORT_SERVERFILE;

    QFile modeFile(modeFileName);
    if (!modeFile.open(QIODevice::ReadOnly))
    {
        // Opening the file failed -- maybe the file is edited at this moment
        // Wait for a moment, then try again

        // TODO: Better check for lock file.

        RTI->log("WARNING: Opening mode file failed. Retrying in 2 sec.");
        Sleep(2000);

        if (!modeFile.open(QIODevice::ReadOnly))
        {
            error=true;
            errorText="Could not read mode file (file locked).";
        }
    }

    if (!error)
    {
        modeFile.close();

        // First read the server file to learn what server this is
        {
            QSettings serverFileIni(serverFileName, QSettings::IniFormat);

            serverName=serverFileIni.value("YarraServer/Name", ORT_INVALID).toString();
            if (serverName==ORT_INVALID)
            {
                error=true;
                errorText="Server file content is not valid.";
            }
            serverType=serverFileIni.value("YarraServer/Type", "").toString().toLower();
        }

        // Now we should be safe, so read the file
        QSettings modeFileIni(modeFileName, QSettings::IniFormat);

        // Read the settings from the file
        int modeCount=0;
        QString modeName="";
        bool finished=false;

        // First estimate the number of modes and mode names
        while (!finished)
        {
            modeName=modeFileIni.value("Modes/"+QString::number(modeCount), ORT_INVALID).toString();

            if ((modeName!=ORT_INVALID) && (modeCount<65000))
            {
                modeCount++;

                ortModeEntry* newEntry=new ortModeEntry;
                newEntry->idName=modeName;

                modes.append(newEntry);
            }
            else
            {
                finished=true;
            }
        }

        count=modeCount;

        // Now read the information for the different modes
        for (int i=0; i<modeCount; i++)
        {
            modeName=modes.at(i)->idName;

            modes.at(i)->protocolTag     =modeFileIni.value(modeName+"/Tag",              ORT_INVALID).toString();
            modes.at(i)->readableName    =modeFileIni.value(modeName+"/Name",             ORT_INVALID).toString();

            QStringList tempList=modeFileIni.value(modeName+"/ConfirmationMail", "").toStringList();
            modes.at(i)->mailConfirmation=tempList.join(",");

            modes.at(i)->requiresACC       =modeFileIni.value(modeName+"/RequiresACC",        true).toBool();
            modes.at(i)->requiresAdjScans  =modeFileIni.value(modeName+"/RequiresAdjScans",   false).toBool();
            modes.at(i)->minimumSizeMB     =modeFileIni.value(modeName+"/MinimumSizeMB",      ORT_MINSIZEMB).toDouble();
            modes.at(i)->requiredServerType=modeFileIni.value(modeName+"/RequiredServerType", "").toString();

            // Read first user parameter
            modes.at(i)->paramLabel      =modeFileIni.value(modeName+"/ParamLabel",       "").toString();
            modes.at(i)->paramDescription=modeFileIni.value(modeName+"/ParamDescription", "").toString();
            modes.at(i)->paramDefault    =modeFileIni.value(modeName+"/ParamDefault",     1).toDouble();
            modes.at(i)->paramMin        =modeFileIni.value(modeName+"/ParamMin",         0).toDouble();
            modes.at(i)->paramMax        =modeFileIni.value(modeName+"/ParamMax",         999999).toDouble();
            modes.at(i)->paramIsFloat    =modeFileIni.value(modeName+"/ParamIsFloat",     false).toBool();

            // If the parameter is an integer parameter, then round all values to integers
            if (!modes.at(i)->paramIsFloat)
            {
                modes.at(i)->paramDefault=int(modes.at(i)->paramDefault);
                modes.at(i)->paramMin    =int(modes.at(i)->paramMin);
                modes.at(i)->paramMax    =int(modes.at(i)->paramMax);
            }

            // Read second user parameter
            modes.at(i)->param2Label      =modeFileIni.value(modeName+"/Param2Label",       "").toString();
            modes.at(i)->param2Description=modeFileIni.value(modeName+"/Param2Description", "").toString();
            modes.at(i)->param2Default    =modeFileIni.value(modeName+"/Param2Default",     1).toDouble();
            modes.at(i)->param2Min        =modeFileIni.value(modeName+"/Param2Min",         0).toDouble();
            modes.at(i)->param2Max        =modeFileIni.value(modeName+"/Param2Max",         999999).toDouble();
            modes.at(i)->param2IsFloat    =modeFileIni.value(modeName+"/Param2IsFloat",     false).toBool();

            // If the parameter is an integer parameter, then round all values to integers
            if (!modes.at(i)->param2IsFloat)
            {
                modes.at(i)->param2Default=int(modes.at(i)->param2Default);
                modes.at(i)->param2Min    =int(modes.at(i)->param2Min);
                modes.at(i)->param2Max    =int(modes.at(i)->param2Max);
            }
        }
    }

    if (error)
    {
        RTI->log("Error during reading of mode file.");
        RTI->log("ERROR: "+errorText);

        QMessageBox msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText("An error occured while reading the Yarra configuration:\n"+errorText+"\n\nPlease check the configuration.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(ORT_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        return false;
    }
    else
    {
        // Reading the mode file was successful
        return true;
    }
}


