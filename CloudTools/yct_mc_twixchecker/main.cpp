#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <stdio.h>

#include <QCoreApplication>
#include <QDir>
#include <QString>

#include "../yct_prepare/yct_twix_anonymizer.h"


#define YMC_TWIXCHECKER_VER "0.1b2"


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\n");
        printf("YarraCloud MC -- Twix Checker %s\n", YMC_TWIXCHECKER_VER);
        printf("-----------------------------------\n\n");
        printf("Usage:    twixchecker [path]\n");
        printf("Purpose:  Checks the Twix (.dat) files provided in the folder [path] for anonymization and consistency.\n");
        return 0;
    }

    QString inputPathString=QString::fromLocal8Bit(argv[1]);
    QDir inputPath(inputPathString);

    if (!inputPath.exists())
    {
        printf("Path does not exist.\n");
        return 1;
    }

    inputPath.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QStringList nameFilters;
    nameFilters << "meas_*.dat";
    inputPath.setNameFilters(nameFilters);

    QFileInfoList fileList = inputPath.entryInfoList();

    if (!fileList.count())
    {
        printf("No Twix files found.\n");
        return 1;
    }

    int minSize = 10*1024*1024;
    int validFileCount = 0;
    QString scannerSerial="";
    QString patientWeight="";
    QString patientSex="";

    for (int i = 0; i < fileList.size(); ++i)
    {
        QFileInfo fileInfo = fileList.at(i);
        if (fileInfo.size() < minSize)
        {
            printf("Implausible size of file\n%s\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }

        // Use the YCT anonymizer class for parsing the files and check if the files might contain PHI
        yctTWIXAnonymizer anonymizer;
        anonymizer.testing = true;
        anonymizer.setStrictVersionChecking(true);
        anonymizer.readAdditionalPatientInformation=true;

        if (!anonymizer.processFile(inputPath.absoluteFilePath(fileInfo.fileName()), "", "", "", "", "", false))
        {
            printf("Invalid Twix file\n%s\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }

        /*
        printf("Name = %s\n", anonymizer.patientInformation.name.toStdString().c_str());
        printf("Serial = %s\n", anonymizer.patientInformation.serialNumber.toStdString().c_str());
        printf("Weight = %s\n", anonymizer.patientInformation.patientWeight.toStdString().c_str());
        printf("Sex = %s\n", anonymizer.patientInformation.patientSex.toStdString().c_str());
        */

        if (!anonymizer.patientInformation.name.contains("xxxxxxxxxx"))
        {
            printf("Error: Files not anonymized (PatientName)\n%s\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }

        if (!anonymizer.patientInformation.mrn.contains("xxxxxxxx"))
        {
            printf("Error: Files not anonymized (PatientID)\n%s\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }

        if ((!scannerSerial.isEmpty()) && (scannerSerial!=anonymizer.patientInformation.serialNumber))
        {
            printf("Error: Files do not belong together (SerialNumer)\n%s\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }
        scannerSerial=anonymizer.patientInformation.serialNumber;

        if ((!patientWeight.isEmpty()) && (patientWeight!=anonymizer.patientInformation.patientWeight))
        {
            printf("Error: Files do not belong together (PatientWeight)\n%s\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }
        patientWeight=anonymizer.patientInformation.patientWeight;

        if ((!patientSex.isEmpty()) && (patientSex!=anonymizer.patientInformation.patientSex))
        {
            printf("Error: Files do not belong together (PatientSex)\n%s\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }
        patientSex=anonymizer.patientInformation.patientSex;

        validFileCount++;
    }

    if (validFileCount!=4)
    {
        printf("Error: Invalid number of files (required 5, found %d)\n", validFileCount);
        return 1;
    }

    return 0;
}
