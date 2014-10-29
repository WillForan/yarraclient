#include <QCoreApplication>
#include <iostream>
#include <stdio.h>

#include "../Client/rds_checksum.h"

int main(int argc, char *argv[])
{
    printf("\nYarra MD5 Checksum Verification\n");
    printf("-------------------------------\n\n");

    if (argc != 2)
    {
        printf("Usage: ChecksumVerify [filename.dat]\n\n");
    }
    else
    {
        QString filename=QString::fromLocal8Bit(argv[1]);

        if (!QFile::exists(filename))
        {
            printf("File not found (%s)\n\n",qPrintable(filename));
        }
        else
        {
            QTime t;
            t.start();

            QString checksum=rdsChecksum::getChecksum(filename);
            printf("MD5 checksum is %s\n\n",qPrintable(checksum));

            qDebug("Time elapsed: %d ms", t.elapsed());

        }
    }

    return 0;
}
