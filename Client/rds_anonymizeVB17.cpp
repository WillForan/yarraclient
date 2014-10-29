#include "rds_anonymizeVB17.h"
#include "rds_global.h"


#define MAX_LINE_LENGTH 1024


rdsAnonymizeVB17::rdsAnonymizeVB17()
{
}


bool rdsAnonymizeVB17::anonymize(QString filename)
{
    rdsAnonymizeVB17* instance=new rdsAnonymizeVB17();

    bool result=instance->perform(filename);

    delete instance;
    instance=0;

    return result;
}


bool rdsAnonymizeVB17::perform(QString filename)
{
    if (!QFile::exists(filename))
    {
        RTI->log("ERROR: Couldn't find file "+filename+".");
        RTI->log("ERROR: This file will NOT be anonymized!");
        return false;
    }

    // First, it is tested if the file is actually a VB17 TWIX file

    // ------------------
    // Open the file
    // ------------------
    QFile inputFile(filename);
    inputFile.open(QIODevice::ReadOnly);

    // ------------------
    // Read the first line and decide if this is a supported TWIX file
    // ------------------

    // Move behind the header size information, which is encoded in binary form
    // at the beginning of the file (this data might contain a LF char, which might
    // be misinterpreted by readLine)
    inputFile.seek(19);

    int fileType=TWIX_INVALID;
    QByteArray firstLine = inputFile.readLine(MAX_LINE_LENGTH);

    if (firstLine.indexOf("<XProtocol>", 0) != -1)
    {
        // File contains VB17 TWIX data
        fileType=TWIX_VB17;
    }
    else
    {
        RTI->log("ERROR: File is not a supported TWIX file (VB17).");
        fileType=TWIX_INVALID;
    }

    inputFile.close();

    if (fileType==TWIX_VB17)
    {
        // File is a VB17 file. Call the actual anonymization function.
        return anonymizeVB17(filename);
    }
    else
    {
        return false;
    }
}


bool rdsAnonymizeVB17::anonymizeVB17(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadWrite);

    // Read the size of the header from the first 32 bytes
    uint32_t headerSize=0;
    file.read((char*) &headerSize, qint64(sizeof(uint32_t)));

    int lineStart=file.pos();
    QByteArray currentLine=file.readLine(MAX_LINE_LENGTH);
    int lineEnd=file.pos();

    bool terminate=false;
    int charsToRead=0;
    bool isLineValid=false;

    bool waitingForSensitiveData=false;

    while (!terminate)
    {
        isLineValid=false;

        lineStart=file.pos();
        charsToRead=MAX_LINE_LENGTH;
        if ((uint32_t) lineStart+charsToRead>headerSize)
        {
            charsToRead=headerSize-lineStart;
        }

        if (charsToRead>2)
        {
            currentLine = file.readLine(charsToRead);

            if (currentLine.size()<charsToRead)
            {
                isLineValid=true;
            }
        }
        else
        {
            terminate=true;
            //addLog("Stopped at position " + QString::number(lineEnd));
        }
        lineEnd=file.pos();

        int result=NO_SENSITIVE_INFORMATION;

        if (isLineValid)
        {
            if (waitingForSensitiveData)
            {
                // It is expected that this line contains sensitive information
                // after a line break
                result=analyzeFollowLine(&currentLine);

                if ((result==SENSITIVE_INFORMATION_CLEARED) || (result==NO_SENSITIVE_INFORMATION))
                {
                    // Sensitive information found, continue normally for next line
                    waitingForSensitiveData=false;
                }
            }
            else
            {
                // Process line
                result=analyzeLine(&currentLine);

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
                file.seek(lineStart);
                file.write(currentLine);

                // Check whether writing the file was successful
                if (file.pos() != lineEnd)
                {
                    RTI->log("WARNING: File inconsistency detected during anonymization!");
                    RTI->log("WARNING: File name = " + filename);
                }
            }
        }

        if (((uint32_t) lineEnd>=headerSize) || (file.atEnd()))
        {
            terminate=true;
            //addLog("Stopped at position " + QString::number(lineEnd));
            //addLog("Line start is " + QString::number(lineStart));
        }
    }

    file.close();

    return true;
}


int rdsAnonymizeVB17::analyzeLine(QByteArray* line)
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


int rdsAnonymizeVB17::clearLine(QByteArray* line)
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
        for (int i=startPos+3; i<endPos; i++)
        {
            line->replace(i,1,"X");
        }
        break;

    case CONTENT_BIRTHDAY:
        line->replace(startPos+3,8,"20100101");
        break;

    case CONTENT_ID:
        for (int i=startPos+3; i<endPos; i++)
        {
            line->replace(i,1,"0");
        }
        break;
    }

    return SENSITIVE_INFORMATION_CLEARED;
}


int rdsAnonymizeVB17::analyzeFollowLine(QByteArray* line)
{
    // Check if line contains closing of brackets
    if (line->indexOf("}", 0)>=0)
    {
        return NO_SENSITIVE_INFORMATION;
    }

    // Check if line contains new XML tag
    if (line->indexOf("<", 0)>=0)
    {
        RTI->log("ERROR: File inconsistency while expecting sensitive information!");
        RTI->log("ERROR: Affected line: "+QString(*line));
        addedLogMessage=true;
        return NO_SENSITIVE_INFORMATION;
    }

    int startPos=line->indexOf("\"", 0);
    int endPos=line->indexOf("\"", startPos+1);

    if ((startPos>=0) && (endPos<=startPos))
    {
        RTI->log("ERROR: Inconsistent information found!");
        return NO_SENSITIVE_INFORMATION;
    }

    if ((startPos>=0) && (endPos>startPos))
    {
        // Information found to be cleared

        switch (expectedContent)
        {
        case CONTENT_NAME:
        default:
            for (int i=startPos+1; i<endPos; i++)
            {
                line->replace(i,1,"X");
            }
            break;

        case CONTENT_BIRTHDAY:
            line->replace(startPos+1,8,"20100101");
            break;
        }

        return SENSITIVE_INFORMATION_CLEARED;
    }

    // Senstive information still not found in this line
    return SENSITIVE_INFORMATION_FOLLOWS;
}

