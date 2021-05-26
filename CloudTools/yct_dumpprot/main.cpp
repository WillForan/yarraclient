#include <QCoreApplication>
#include <QDir>
#include <QString>

#include "../yct_prepare/yct_twix_anonymizer.h"

#define YCT_DUMPPROT_VER "0.1b4"


int main(int argc, char *argv[])
{
    printf("\n");

    if (argc < 3)
    {
        printf("Yarra Client Tools - Dump Protocol %s\n",YCT_DUMPPROT_VER);
        printf("----------------------------------------\n\n");
        printf("Usage:    yct_dumpprot dump [twixfile]\n");
        printf("Purpose:  Extract the scan protocol from a Twix file into a text file (with extension .prot)\n\n");
        printf("Usage:    yct_dumpprot info [twixfile]\n");
        printf("Purpose:  Show information about the protocols contained in the Twix file\n");
        return 0;
    }   

    QString filename=QString::fromLocal8Bit(argv[2]);
    if (!QFile::exists(filename))
    {
        printf("File not found.\n");
        return 1;
    }

    yctTWIXAnonymizer anonymizer;
    anonymizer.testing=true;
    anonymizer.setStrictVersionChecking(false);

    std::string cmd(argv[1]);
    if (cmd=="info")
    {
        anonymizer.showOnlyInfo=true;
        qInfo() << "Filename:" << filename;
    }
    else
    if (cmd=="dump")
    {
        anonymizer.dumpProtocol=true;
    }
    else
    {
        printf("Error! Unknown option\n");
        return 1;
    }

    if (!anonymizer.processFile(filename,"","","","","",false))
    {
        qInfo() << "";
        qInfo() << "Error! Unable to parse file " << filename;
        return 1;
    }

    if (cmd=="dump")
    {
        printf("Protocol written to .prot file.\n");
    }
    return 0;
}
