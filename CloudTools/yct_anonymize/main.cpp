#include <QCoreApplication>
#include <QDir>
#include <QString>

#include "../yct_prepare/yct_twix_anonymizer.h"

#define YCT_ANONYMIZER_VER "0.2b5"


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


int main(int argc, char *argv[])
{
    printf("\nYarra Client Tools - Batch Anonymizer %s\n", YCT_ANONYMIZER_VER);
    printf("-------------------------------------------\n\n");

    if (argc < 3)
    {
        printf("Usage:    yct_anonymizer [input path] [output path] [optional: patient-name replacement]\n\n");
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

        QFileInfoList fileList=inDir.entryInfoList(QStringList("*.dat"),QDir::Files,QDir::Time);
        if (fileList.empty())
        {
            printf("Nothing to do.\n");
            return 0;
        }

        // Open or create the CSV file and add the header to it
        QFile tableFile;
        tableFile.setFileName("files.csv");
        tableFile.open(QIODevice::Append | QIODevice::Text);
        tableFile.write(phiEntry::getCSVHeader().toLatin1());

        bool error=false;
        QString replaceName="";

        if (argc>=4)
        {
            replaceName=QString::fromLocal8Bit(argv[3]);
            qInfo() << "Using replacement name: " << replaceName;
            qInfo() << "";
        }

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

            qInfo() << fileInfo.fileName() << " --> " << destFilename;

            yctTWIXAnonymizer anonymizer;
            anonymizer.patientInformation.fillStr=uuidChars;
            if (!replaceName.isEmpty())
            {
                anonymizer.patientInformation.fillStr=replaceName;
            }

            if (!QFile::copy(fileName,outDir.absoluteFilePath(destFilename)))
            {
                qInfo() << "Error! Unable to copy file " << fileName << " to " << destFilename;
                error=true;
                break;
            }

            if (!anonymizer.processFile(outDir.absoluteFilePath(destFilename),"none.phi","","","","",false))
            {
                qInfo() << "Error! Unable to anonymize file " << destFilename;

                if (!QFile::remove(outDir.absoluteFilePath(destFilename)))
                {
                    qInfo() << "Error! Unable to remove file " << destFilename;
                }

                error=true;
                break;
            }

            // Store the phi information in a text file in CSV format
            phiEntry entry;
            entry.originalFilename=fileInfo.fileName();
            entry.anonymizedFilename=destFilename;
            entry.uuid=uuid;
            entry.patientName=anonymizer.patientInformation.name;
            entry.dob=anonymizer.patientInformation.dateOfBirth;
            entry.mrn=anonymizer.patientInformation.mrn;
            tableFile.write(entry.getCSVLine().toLatin1());
        }

        tableFile.close();

        if (error)
        {
            return 1;
        }
    }

    printf("\n");
    printf("Please use the data with caution!\n");
    printf("The authors of this tool take no responsibility of any kind.\n");

    return 0;
}

