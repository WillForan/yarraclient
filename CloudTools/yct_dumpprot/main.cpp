#include <QCoreApplication>
#include <QDir>
#include <QString>

#include "../yct_prepare/yct_twix_anonymizer.h"

#define YCT_DUMPPROT_VER "0.1b3"


int main(int argc, char *argv[])
{
    printf("\nYarra Client Tools - Dump Protocol %s\n",YCT_DUMPPROT_VER);
    printf("----------------------------------------\n\n");

    if (argc < 2)
    {
        printf("Usage:    yct_dumpprot [twixfile]\n\n");
        printf("Purpose:  Extract the scan protocol from a Twix file into a text file (with extension .prot)\n");
        return 0;
    }
    else
    {
        QString filename=QString::fromLocal8Bit(argv[1]);

        if (!QFile::exists(filename))
        {
            printf("File not found.\n");
            return 1;
        }

        yctTWIXAnonymizer anonymizer;
        anonymizer.testing=true;
        anonymizer.dumpProtocol=true;
        anonymizer.setStrictVersionChecking(false);

        if (!anonymizer.processFile(filename,"","","","","",false))
        {
            qInfo() << "";
            qInfo() << "Error! Unable to parse file " << filename;
            return 1;
        }
    }

    printf("Protocol written to .prot file.\n");

    return 0;
}

