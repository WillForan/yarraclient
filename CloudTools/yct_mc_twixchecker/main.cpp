#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <stdio.h>

#include <QCoreApplication>
#include <QDir>
#include <QString>

#include "../yct_prepare/yct_twix_anonymizer.h"


#define YMC_TWIXCHECKER_VER "0.1b1"


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

    QString scannerSerial="";
    int minSize = 10*1024*1024;

    for (int i = 0; i < fileList.size(); ++i)
    {
        QFileInfo fileInfo = fileList.at(i);
        if (fileInfo.size() < minSize)
        {
            printf("Implausible size of file %s.\n", fileInfo.fileName().toStdString().c_str());
            return 1;
        }

        // Use the YCT anonymizer class for parsing the files and check if the files might contain PHI
        yctTWIXAnonymizer anonymizer;
        anonymizer.testing = true;
        anonymizer.setStrictVersionChecking(true);

        printf("%s.\n", fileInfo.fileName().toStdString().c_str());

        /*
        if (!anonymizer.processFile(fileInfo.fileName(), "", "", "", "", "", false))
        {
            return 1;
        }
        */
    }

    return 0;
}
