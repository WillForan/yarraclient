
#include <QtCore>

#include <iostream>
#include <stdio.h>

#include "yct_twix_anonymizer.h"


int main(int argc, char *argv[])
{
    printf("\nYarra CloudTools - Rawdata Preprocessor %s\n", YCT_TWIXANONYMIZER_VER);
    printf("--------------------------------------------\n\n");

    if (argc < 4)
    {
        printf("Usage:   yct_prepare [filename.dat] [path for filename.phi] [acc] [options]\n\n");
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

        QDir phiDir(phiPath);
        if (!phiDir.exists())
        {
            printf("Output path for phi file not found (%s)\n\n", qPrintable(phiPath));
            return 1;
        }

        yctTWIXAnonymizer twixAnonymizer;

        QString options="";
        for (int i=4; i<argc; i++)
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

        QString uuid=QUuid::createUuid().toString();
        QString acc=QString::fromLocal8Bit(argv[3]);
        QFileInfo filename(rawFilename);
        QString taskid=filename.completeBaseName();

        return twixAnonymizer.processFile(rawFilename,phiPath,acc,taskid,uuid);
    }
}
