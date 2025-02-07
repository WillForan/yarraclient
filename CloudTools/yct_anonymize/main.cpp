#include <QCoreApplication>
#include <QDir>
#include <QString>

#include "../yct_prepare/yct_twix_anonymizer.h"

#define YCT_ANONYMIZER_VER "0.2b11"


class phiEntry
{
public:
    QString anonymizedFilename;
    QString originalFilename;
    QString uuid;
    QString patientName;
    QString dob;
    QString mrn;

    QString encl(QString entry)
    {
        return "\""+entry+"\"";
    }

    static QString getCSVHeader()
    {
        // Create header with separator information, so that Excel is able to display the CSV correctly
        return "sep=,\n\"Anonymized File\",\"Original File\",\"UUID\",\"Patient Name\",\"DOB\",\"MRN\",\"Processing Time\"\n";
    }

    QString getCSVLine()
    {
        return encl(anonymizedFilename)+","+
               encl(originalFilename)+","+
               uuid+","+
               encl(patientName)+","+
               dob+","+
               mrn+","+
               QDateTime::currentDateTime().toString()+
               "\n";
    }
};


bool processFolder(QDir currentInput, QDir currentOutput, QString inputPathPrefix, QString outputPathPrefix, int recursionDepth, QString replaceName, QFile* tableFile, bool strictVersionCheck=true)
{
    if (recursionDepth > 100)
    {
        qInfo() << "Warning: Too many nested subfolders (> 100). Stopping processing at " << currentInput.absolutePath();
        return true;
    }

    QString indent = "";
    indent.fill(' ', 4*recursionDepth);

    // First loop over all files (ignoring existing folders)
    QFileInfoList fileList=currentInput.entryInfoList(QStringList("*.dat"),QDir::Files,QDir::Name);

    bool success=true;
    while (!fileList.empty())
    {
        QFileInfo fileInfo=fileList.takeFirst();
        QString fileName=fileInfo.absoluteFilePath();

        QString uuid=QUuid::createUuid().toString();
        // Remove the curly braces enclosing the id
        uuid=uuid.mid(1,uuid.length()-2);

        QString destFilename=uuid+".dat";
        QString uuidChars=uuid;
        uuidChars.remove(QChar('-'),Qt::CaseSensitive);

        qInfo() << indent.toLatin1().data() << "*" << fileInfo.fileName() << "-->" << destFilename;

        yctTWIXAnonymizer anonymizer;
        anonymizer.setStrictVersionChecking(strictVersionCheck);
        anonymizer.patientInformation.fillStr=uuidChars;
        if (!replaceName.isEmpty())
        {
            anonymizer.patientInformation.fillStr=replaceName;
        }

        if (!QFile::copy(fileName,currentOutput.absoluteFilePath(destFilename)))
        {
            qInfo() << "Error! Unable to copy file " << fileName << " to " << destFilename;
            success=false;
            break;
        }

        if (!anonymizer.processFile(currentOutput.absoluteFilePath(destFilename),"none.phi","","","","",false))
        {
            qInfo() << "Error! Unable to anonymize file " << destFilename;

            if (!QFile::remove(currentOutput.absoluteFilePath(destFilename)))
            {
                qInfo() << "Error! Unable to remove file " << destFilename;
            }

            success=false;
            break;
        }

        // Store the phi information in a text file in CSV format
        phiEntry entry;
        entry.originalFilename=inputPathPrefix+fileInfo.fileName();
        entry.anonymizedFilename=outputPathPrefix+destFilename;
        entry.uuid=uuid;
        entry.patientName=anonymizer.patientInformation.name;
        entry.dob=anonymizer.patientInformation.dateOfBirth;
        entry.mrn=anonymizer.patientInformation.mrn;
        tableFile->write(entry.getCSVLine().toLatin1());
    }

    // Now loop over existing folders and process recursively    
    int folderCounter = 0;
    QFileInfoList folderList=currentInput.entryInfoList(QStringList("*"), QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    while (!folderList.empty())
    {
        QFileInfo folderInfo=folderList.takeFirst();

        QDir inFolder = currentInput;
        if (!inFolder.cd(folderInfo.fileName()))
        {
            qInfo() << "Error! Unable to enter input folder " <<  folderInfo.fileName();
            success=false;
            break;
        }

        // Generate neutral name for subfolders
        folderCounter++;
        QString newOutputFolder = (QString("%1").arg(QString::number(folderCounter), 4, QLatin1Char('0'))).toLatin1();

        qInfo() << "";
        qInfo() << indent.toLatin1().data() << "> Folder" << folderInfo.fileName() << "-->" << newOutputFolder;

        if (!currentOutput.mkdir(newOutputFolder))
        {
            qInfo() << "Error! Unable to create subfolder in output path " << newOutputFolder;
            success=false;
            break;
        }

        QDir outFolder = currentOutput;
        if (!outFolder.cd(newOutputFolder.toLatin1()))
        {
            qInfo() << "Error! Unable to enter output folder " <<  newOutputFolder;
            success=false;
            break;
        }

        QString inputRecursionPath = inputPathPrefix + folderInfo.fileName() + "\\";
        QString outputRecursionPath = outputPathPrefix + newOutputFolder + "\\";
        if (!processFolder(inFolder, outFolder, inputRecursionPath, outputRecursionPath, recursionDepth+1, replaceName, tableFile))
        {
            qInfo() << "Error! Unable to process folder " <<  inputRecursionPath << "-->" << outputRecursionPath;
            success=false;
            break;
        }
    }

    return success;
}


int main(int argc, char *argv[])
{
    printf("\nYarra Client Tools - Batch Anonymizer %s\n", YCT_ANONYMIZER_VER);
    printf("-------------------------------------------\n\n");

    if (argc < 3)
    {
        printf("Usage:    yct_anonymizer [input path] [output path] [optional: patient-name replacement]\n\n");
        printf("");
        printf("Purpose:  Anonymizes all Twix files located in [input path]. The anonymized files will be \n");
        printf("          written into [output path]. Each anonymized file will be named by a unique ID (UUID).\n");
        printf("          The patient name will be replaced by the UUID (while keeping the original length).\n");
        printf("          If a name is provided as 3rd parameter, this name will be used instead to replace\n");
        printf("          the patient name. A file in CSV format will be created (files.csv) that lists the\n");
        printf("          original and anonymized file names, as well as the removed PHI.\n");
        printf("\n");
        printf("Note:     Please use this tool with caution. Operation of the software is at the user's\n");
        printf("          own risk. The authors take no responsibility of any kind.\n");

        return 0;
    }
    else
    {
        bool useStrictVersionCheck=true;
        QString inPath =QString::fromLocal8Bit(argv[1]);
        QString outPath=QString::fromLocal8Bit(argv[2]);

        QDir inDir;
        if (!inDir.cd(inPath))
        {
            printf("Input path doesn't exist.\n");
            return 1;
        }

        QDir outDir(outPath);
        if (!outDir.exists())
        {
            printf("Output path doesn't exist.\n");
            return 1;
        }

        QString replaceName="";
        if (argc>=4)
        {
            if (QString::fromLocal8Bit(argv[3]) != "-disable-version-check")
            {
                replaceName=QString::fromLocal8Bit(argv[3]);
                qInfo() << "Using replacement name: " << replaceName;
                qInfo() << "";
            }
        }

        // Secret option to disable the version check
        if ((argc>=4) && (QString::fromLocal8Bit(argv[argc-1]) == "-disable-version-check"))
        {
            useStrictVersionCheck=false;
        }

        // Open or create the CSV file and add the header to it
        QFile tableFile;
        tableFile.setFileName("files.csv");
        tableFile.open(QIODevice::Append | QIODevice::Text);
        tableFile.write(phiEntry::getCSVHeader().toLatin1());

        // Recursively process the input folder
        bool success=processFolder(inDir, outDir, "", "", 0, replaceName, &tableFile, useStrictVersionCheck);

        // Close the CSV file in any case (error / no error)
        tableFile.close();

        // Indicate error in exit code
        if (!success)
        {
            return 1;
        }
    }

    printf("\n\n");
    printf("Please use the data with caution!\n");
    printf("The authors of this tool take no responsibility of any kind.\n");

    return 0;
}

