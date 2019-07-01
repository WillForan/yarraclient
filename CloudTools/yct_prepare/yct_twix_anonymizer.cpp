
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <stdio.h>

#include "yct_twix_anonymizer.h"
#include "yct_twix_header.h"
#include "../yct_common.h"


#define MAX_LINE_LENGTH 65536

#define LOG(x) {std::cout << x << std::endl;}
#define DBG(x) if (debug) {std::cout << x << std::endl;}


yctTWIXAnonymizer::yctTWIXAnonymizer()
{
    errorReason="";
    debug=false;
    testing=false;
    dumpProtocol=false;

    fileVersion=UNKNOWN;
}


bool yctTWIXAnonymizer::processFile(QString twixFilename, QString phiPath,
                                    QString acc, QString taskid, QString uuid, QString mode,
                                    bool storePHI)
{
    bool result=false;

    // Clean the patient data - to be sure
    patientInformation.name       ="";
    patientInformation.mrn        ="";
    patientInformation.dateOfBirth="";

    // Populate the externally provided information
    patientInformation.uuid       =uuid;
    patientInformation.acc        =acc;
    patientInformation.taskid     =taskid;
    patientInformation.mode       =mode;

    QFile file(twixFilename);
    if (!file.open(QIODevice::ReadWrite))
    {
        errorReason="Unable to open raw-data file";
        return false;
    }

    // Determine TWIX file type: VA/VB or VD/VE?
    uint32_t x[2];
    file.read((char*)x, 2*sizeof(uint32_t));

    if ((x[0]==0) && (x[1]<=64))
    {
        fileVersion=VDVE;
        DBG("File type: VD/VE");
    }
    else
    {
        fileVersion=VAVB;
        DBG("File type: VA/VB");
    }
    file.seek(0);

    if (dumpProtocol)
    {
        QString dumpFilename=twixFilename+".prot";

        dumpFile.setFileName(dumpFilename);
        if (!dumpFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            LOG("ERROR: Unable to create dump file " << dumpFilename.toStdString());
            return false;
        }
    }

    // If VD/VE format, there can be multiple measurements stored in the same file
    if (fileVersion==VDVE)
    {
        uint32_t id=0, ndset=0;
        std::vector<VD::EntryHeader> veh;

        file.read((char*)&id,   sizeof(uint32_t));  // ID
        file.read((char*)&ndset,sizeof(uint32_t));  // # data sets

        DBG("");
        DBG("Contained datasets: " << ndset);
        DBG("");

        if ((ndset>30) || (ndset<1))
        {
            // If there are more than 30 measurements, it's unlikely that the
            // file is a valid TWIX file
            LOG("WARNING: Number of measurements in file " << ndset);

            errorReason="File is invalid (invalid number of measurements)";
            file.close();
            return false;
        }

        veh.resize(ndset);

        // TODO: Remove patient name from index block
        for (size_t i=0; i<ndset; ++i)
        {
            qint64 startPos=file.pos();
            file.read((char*)&veh[i], VD::ENTRY_HEADER_LEN);            
            qint64 endPos=file.pos();

            DBG(veh.at(i).PatientName);
            DBG(veh.at(i).ProtocolName);

            // Clean the patient name
            if (!testing)
            {
                // Wipe the whole array to be sure
                memset(veh.at(i).PatientName,0,64);
                strcpy(veh.at(i).PatientName,"YARRAYARRA");
                file.seek(startPos);
                file.write((char*)&veh[i], VD::ENTRY_HEADER_LEN);

                // Check whether writing the file was successful
                if (file.pos() != endPos)
                {
                    LOG("WARNING: File inconsistency detected while cleaning measurement table!");
                }
            }
        }

        DBG("");

        // Process the individual measurements in the file
        for (size_t i=0; i<ndset; ++i)
        {
            // Jump to ith measurement
            file.seek(veh.at(i).MeasOffset);
            result=processMeasurement(&file);

            if (!result)
            {
                LOG("ERROR: Error while processing section " << i);
                break;
            }
        }
    }
    else
    {
        result=processMeasurement(&file);
    }    

    file.close();    

    if (dumpProtocol)
    {
        dumpFile.close();
    }

    if (storePHI)
    {
        if (!checkAndStorePatientData(twixFilename, phiPath))
        {
            return false;
        }
    }

    return result;
}


bool yctTWIXAnonymizer::processMeasurement(QFile* file)
{
    uint32_t headerLength=0;
    uint64_t headerEnd   =0;
    qint64   headerStart=file->pos();

    // Find header length
    file->read((char*)&headerLength, sizeof(uint32_t));

    if ((headerLength<=0) || (headerLength>5000000))
    {
        // File header is invalid

        std::string fileType="VB";
        if (fileVersion==VDVE)
        {
            fileType="VD/VE";
        }
        LOG("WARNING: Unusual header size " << headerLength << " (file type " << fileType << ")");

        errorReason="File is invalid (unusual header size)";
        return false;
    }

    // Parse header
    DBG("Header size is " << headerLength << " bytes.");
    DBG("");

    headerEnd=(uint64_t) headerStart+headerLength;

    QByteArray line="";
    qint64     lineStart=0;
    qint64     lineEnd  =0;
    bool       isLineValid=false;
    bool       waitingForSensitiveData=false;
    bool       terminateParsing=false;
    int        charsToRead=0;
    //int      maxRead=0;

    while ((!file->atEnd()) && (file->pos()<headerEnd) && (!terminateParsing))
    {        
        isLineValid=false;
        lineStart=file->pos();
        charsToRead=MAX_LINE_LENGTH;
        if (lineStart+charsToRead>=headerEnd)
        {
            charsToRead=headerEnd-lineStart;
        }

        if (charsToRead>0)
        {
            line=file->readLine(charsToRead+1);

            if (dumpProtocol)
            {
                dumpFile.write(line);
            }

            //LOG("READ: " << line.size());

            if (line.size()<MAX_LINE_LENGTH)
            {
                isLineValid=true;

                /*
                if (line.size()>maxRead)
                {
                    maxRead=line.size();
                }
                */
            }
            else
            {
                LOG("ERROR: Incomplete line read (exceeds buffer size)");
                LOG("ERROR: Anonymization not possible. Report error to Yarra team.");
                //LOG("Chopped line: charsToRead=" << charsToRead << " actual: " << line.size());
                //LOG(line.data())

                return false;
            }
        }
        else
        {
            terminateParsing=true;
            //DBG("Stopped at position " + QString::number(lineEnd));
        }
        lineEnd=file->pos();

        int result=NO_SENSITIVE_INFORMATION;

        if (isLineValid)
        {
            if (waitingForSensitiveData)
            {
                // It is expected that this line contains sensitive information
                // after a line break
                result=analyzeFollowLine(&line);

                if ((result==SENSITIVE_INFORMATION_CLEARED) || (result==NO_SENSITIVE_INFORMATION))
                {
                    // Sensitive information found, continue normally for next line
                    waitingForSensitiveData=false;
                }
            }
            else
            {
                // Process line
                result=analyzeLine(&line);

                if (result==SENSITIVE_INFORMATION_FOLLOWS)
                {
                    // Sensitive information follows in the next lines
                    waitingForSensitiveData=true;
                }
            }

            if (result==SENSITIVE_INFORMATION_CLEARED)
            {
                // Line contains information that has been anonymized.
                // Write anonymized line back to file.
                if (!testing)
                {
                    file->seek(lineStart);
                    file->write(line);
                }

                // Check whether writing the file was successful
                if (file->pos() != lineEnd)
                {
                    LOG("WARNING: File inconsistency detected during anonymization!");
                }
            }
        }

        if ((lineEnd>=headerEnd) || (file->atEnd()))
        {
            terminateParsing=true;

            //DBG("Stopped at position " + QString::number(lineEnd));
            //DBG("Line start is " + QString::number(lineStart));
        }
    }

    //LOG("INFO: Max line size = " << maxRead);

    return true;
}


bool yctTWIXAnonymizer::checkAndStorePatientData(QString twixFilename, QString phiPath)
{
    DBG("");
    DBG("Patient name:  " + patientInformation.name.toStdString());
    DBG("Date of birth: " + patientInformation.dateOfBirth.toStdString());
    DBG("MRN:           " + patientInformation.mrn.toStdString());
    DBG("ACC:           " + patientInformation.acc.toStdString());
    DBG("UUID:          " + patientInformation.uuid.toStdString());
    DBG("TaskID:        " + patientInformation.taskid.toStdString());
    DBG("Mode:          " + patientInformation.mode.toStdString());
    DBG("");    

    QDir phiDir(phiPath);
    if (!phiDir.exists())
    {
        LOG("ERROR: Unable to access phi path " << phiPath.toStdString());
        return false;
    }

    QFileInfo twixFile(twixFilename);
    QString phiFilename=phiPath+"/"+twixFile.completeBaseName()+".phi";

    if (QFile::exists(phiFilename))
    {
        LOG("WARNING: PHI file already exists. Keeping existing file.");
        return false;
    }

    // If patient information has not been found in the file, set it to a recognizable value
    if (patientInformation.name.isEmpty())
    {
        patientInformation.name="MISSING";
    }
    if (patientInformation.mrn.isEmpty())
    {
        patientInformation.mrn="MISSING";
    }
    if (patientInformation.dateOfBirth.isEmpty())
    {
        patientInformation.dateOfBirth="MISSING";
    }

    QSettings phiFile(phiFilename, QSettings::IniFormat);
    phiFile.setValue("PHI/NAME",   patientInformation.name);
    phiFile.setValue("PHI/DOB",    patientInformation.dateOfBirth);
    phiFile.setValue("PHI/MRN",    patientInformation.mrn);
    phiFile.setValue("PHI/ACC",    patientInformation.acc);
    phiFile.setValue("PHI/UUID",   patientInformation.uuid);
    phiFile.setValue("PHI/TASKID", patientInformation.taskid);
    phiFile.setValue("PHI/MODE",   patientInformation.mode);

    phiFile.setValue(YCT_TIMEPT_CREATED,QDateTime::currentDateTime().toString(Qt::ISODate));

    return true;
}


int yctTWIXAnonymizer::analyzeLine(QByteArray* line)
{
    if (line->indexOf("<ParamString.\"tPatientName\">", 0) > 0)
    {
        expectedContent=CONTENT_NAME;
        return clearLine(line);
    }

    if (line->indexOf("<ParamString.\"PatientName\">", 0) > 0)
    {
        expectedContent=CONTENT_NAME;
        return clearLine(line);
    }

    if (line->indexOf("<ParamString.\"PatientsName\">", 0) > 0)
    {
        expectedContent=CONTENT_NAME;
        return clearLine(line);
    }

    if (line->indexOf("<ParamString.\"PatientBirthDay\">", 0) > 0)
    {
        expectedContent=CONTENT_BIRTHDAY;
        return clearLine(line);
    }

    if (line->indexOf("<ParamString.\"PatientID\">", 0) > 0)
    {
        expectedContent=CONTENT_ID;
        return clearLine(line);
    }

    if (line->indexOf("<ParamString.\"tPerfPhysiciansName\">", 0) > 0)
    {
        expectedContent=CONTENT_NAME;
        return clearLine(line);
    }

    return NO_SENSITIVE_INFORMATION;
}


int yctTWIXAnonymizer::clearLine(QByteArray* line)
{
    int startPos=line->indexOf("{ \"", 0);
    int endPos=line->indexOf("\"  }", 0);

    if (startPos < 0)
    {
        // Information seems to follow after line break
        return SENSITIVE_INFORMATION_FOLLOWS;
    }

    switch (expectedContent)
    {
    case CONTENT_NAME:
    default:
        DBG("Found patient name: " + line->toStdString());

        if (patientInformation.name.isEmpty())
        {
            patientInformation.name=QString(line->mid(startPos+3,endPos-(startPos+3)));
        }

        // If a fill string has been provided, overwrite the patient name with that;
        // otherwise, insert "Y" characters.
        if (patientInformation.fillStr.isEmpty())
        {
            for (int i=startPos+3; i<endPos; i++)
            {
                line->replace(i,1,"Y");
            }
        }
        else
        {
            int j=0;
            for (int i=startPos+3; i<endPos; i++)
            {
                if (j<patientInformation.fillStr.length())
                {
                    const char chr=patientInformation.fillStr.at(j).toLatin1();
                    line->replace(i,1,&chr,1);
                }
                else
                {
                    line->replace(i,1," ");
                }
                j++;
            }
            //std::cout << "Out:" << line->toStdString() << ":" << std::endl;
        }
        break;

    case CONTENT_BIRTHDAY:
        DBG("Found birthday: " + line->toStdString());

        if (patientInformation.dateOfBirth.isEmpty())
        {
            patientInformation.dateOfBirth=QString(line->mid(startPos+3,endPos-(startPos+3)));
        }

        line->replace(startPos+3,8,"20120101");
        break;

    case CONTENT_ID:
        DBG("Found MRN: " + line->toStdString());

        if (patientInformation.mrn.isEmpty())
        {
            patientInformation.mrn=QString(line->mid(startPos+3,endPos-(startPos+3)));
        }

        for (int i=startPos+3; i<endPos; i++)
        {
            line->replace(i,1,"0");
        }
        break;
    }

    return SENSITIVE_INFORMATION_CLEARED;
}


int yctTWIXAnonymizer::analyzeFollowLine(QByteArray* line)
{
    // Check if line contains closing of brackets
    if (line->indexOf("}", 0)>=0)
    {
        return NO_SENSITIVE_INFORMATION;
    }

    if (line->indexOf("<Visible> \"true\"",0)>=0)
    {
        return SENSITIVE_INFORMATION_FOLLOWS;
    }

    // Check if line contains new XML tag
    if (line->indexOf("<", 0)>=0)
    {
        LOG("ERROR: File inconsistency while expecting sensitive information!");
        LOG("ERROR: Affected line: " << line->toStdString());
        errorReason="File inconsistency while expecting sensitive information!";

        return PROCESSING_ERROR;
    }

    int startPos=line->indexOf("\"", 0);
    int endPos=line->indexOf("\"", startPos+1);

    if ((startPos>=0) && (endPos<=startPos))
    {
        LOG("ERROR: Inconsistent information found!");
        errorReason="Inconsistent information found!";

        return PROCESSING_ERROR;
    }

    if ((startPos>=0) && (endPos>startPos))
    {
        // Information found to be cleared

        switch (expectedContent)
        {
        case CONTENT_NAME:
        default:
            // If a fill string has been provided, overwrite the patient name with that;
            // otherwise, insert "Y" characters.
            if (patientInformation.fillStr.isEmpty())
            {
                for (int i=startPos+1; i<endPos; i++)
                {
                    line->replace(i,1,"Y");
                }
            }
            else
            {
                int j=0;
                for (int i=startPos+1; i<endPos; i++)
                {
                    if (j<patientInformation.fillStr.length())
                    {
                        const char chr=patientInformation.fillStr.at(j).toLatin1();
                        line->replace(i,1,&chr,1);
                    }
                    else
                    {
                        line->replace(i,1," ");
                    }
                    j++;
                }
            }
            break;

        case CONTENT_ID:
            for (int i=startPos+1; i<endPos; i++)
            {
                line->replace(i,1,"0");
            }
            break;

        case CONTENT_BIRTHDAY:
            line->replace(startPos+1,8,"20120101");
            break;
        }

        return SENSITIVE_INFORMATION_CLEARED;
    }

    // Senstive information still not found in this line
    return SENSITIVE_INFORMATION_FOLLOWS;
}
