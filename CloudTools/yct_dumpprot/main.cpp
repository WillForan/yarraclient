#include <QCoreApplication>
#include <QDir>
#include <QString>


#include "../yct_prepare/yct_twix_anonymizer.h"



int main(int argc, char *argv[])
{
    printf("\nYarra CloudTools - Dump Protocol\n");
    printf("----------------------------------\n\n");

    if (argc < 2)
    {
        printf("Usage:    yct_dumpprot [twixfile]\n\n");
        printf("Purpose:  Write the protocol from the Twix file into a text file (with extension .prot)\n");
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

        if (!anonymizer.processFile(filename,"","","","","",false))
        {
            qInfo() << "";
            qInfo() << "Error! Unable to parse file " << filename;
            return 1;
        }
    }

    printf("Wrote protocol to .prot file.\n");

    return 0;
}

