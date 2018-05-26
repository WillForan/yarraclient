
#include <QtCore>

#include <iostream>
#include <stdio.h>

#include "yct_twix_anonymizer.h"


int main(int argc, char *argv[])
{
    printf("\nYarra CloudTools - Rawdata Preprocessor %s\n", YCT_TWIXANONYMIZER_VER);
    printf("--------------------------------------------\n\n");

    if (argc < 3)
    {
        printf("Usage:   yct_prepare [filename.dat] [path for filename.phi] [options]\n\n");
        printf("Options: --testing  Only test the processing but don't modify file\n");
        printf("         --debug    Output additional debug information\n");
        printf("         --dump     Dump the protocol from the TWIX file\n");
        printf("\n");

        return 0;
    }
    else
    {
        QString rawFilename =QString::fromLocal8Bit(argv[1]);
        QString phiPath     =QString::fromLocal8Bit(argv[2]);

        if (!QFile::exists(rawFilename))
        {
            printf("TWIX file not found (%s)\n\n", qPrintable(rawFilename));
            return 1;
        }

        QFileInfo twixFile(rawFilename);
        QString taskFilename=twixFile.absolutePath()+"/"+twixFile.completeBaseName()+".task";

        if (!QFile::exists(taskFilename))
        {
            printf("Task file not found (%s)\n\n", qPrintable(taskFilename));
            return 1;
        }

        QDir phiDir(phiPath);
        if (!phiDir.exists())
        {
            printf("Output path for phi file not found (%s)\n\n", qPrintable(phiPath));
            return 1;
        }

        yctTWIXAnonymizer twixAnonymizer;

        QString options="";
        for (int i=3; i<argc; i++)
        {
            options+=QString::fromLocal8Bit(argv[i])+" ";
        }

        if (options.contains("--testing",Qt::CaseInsensitive))
        {
            printf("Using testing mode.\n");
            twixAnonymizer.testing=true;
        }

        if (options.contains("--debug",Qt::CaseInsensitive))
        {
            printf("Using debug mode.\n");
            twixAnonymizer.debug=true;
        }

        if (options.contains("--dump",Qt::CaseInsensitive))
        {
            printf("Dumping protocol into protocol.dmp\n");
            twixAnonymizer.dumpProtocol=true;
        }

        return twixAnonymizer.processFile(rawFilename,taskFilename,phiPath);
    }
}
