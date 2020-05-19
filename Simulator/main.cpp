#include <QtCore>

#include <iostream>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>


static char fileHeadLine[]     = {"\n%7s%11s%32s%32s%9s%13s%13s%22s%22s"};
#if (defined (WINDOWS) || defined (__windows__) || defined(Q_OS_WIN))
    static char fileLine[]       = {"%7u%11u%32s%32s%9s%13I64u%13I64u%22s%22s"};
#else
    static char fileLine[]       = {"%7u%11u%32s%32s%9s%13" PRIu64 "%13" PRIu64 "%22s%22s"};
#endif
static char protName[40] = {""};
static char patName[40] = {""};
static char status[10] = {""};

char filename[200] = {""};

int filesize=10240000;

int fileIDOffset=5100;
int measIDOffset=300;

// 0=info, 1=directory, 2=filecopy
int mode=0;

int main(int argc, char *argv[])
{

    bool nextIsFilename=false;


    for (int i=0; i<argc; i++)
    {
        if (nextIsFilename)
        {
            strcpy(filename,argv[i]);
            nextIsFilename=false;
        }

        if (strcmp(argv[i], "-d")==0)
        {
            mode=1;
        }

        if (strcmp(argv[i], "-m")==0)
        {
            mode=2;
        }

        if (strcmp(argv[i], "-o")==0)
        {
            nextIsFilename=true;
        }

    }

    if (mode==1)
    {
        printf("MrParcThreadPrioImpl singleton created without singleton lock.\n");
        printf("MrParcThreadPrioImpl singleton created without singleton lock.\n");
        printf("\n");
        printf("RAID device\n");
        printf("   97980 Mbyte of  97990 MByte available for I/O\n");
        printf("   97712 MByte of  97980 MByte in use,    268 MByte free\n");
        printf("     882 Files of  24495 Files in use,  23613 Files free\n");
        printf("\n");

        printf (fileHeadLine,
          "FileID",
          "MeasID",
          "ProtName",
          "PatName",
          "Status",
          "Size",
          "SizeOnDisk",
          "CreationTime",
          "CloseTime");
        printf("\n\n");

        strcpy(patName, "Mustermann^Hans");
        strcpy(status, "cld");

        time_t rawtime;
        struct tm * timeinfo;

        char cTimeString[32];
        char fTimeString[32];

        int fileCount=23;
        for (int i=0; i<fileCount; i++)
        {
            time ( &rawtime );
            rawtime=rawtime-i*83;
            timeinfo = localtime ( &rawtime );

            sprintf(fTimeString, "%02d.%02d.%4d %02d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            sprintf(cTimeString, "%02d.%02d.%4d %02d:%02d:%02d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

#define ENRTY(j,a,b) if (i%fileCount==j) { strcpy(patName, a); strcpy(protName, b); };

            ENRTY(0 ,"Lebowski^Jeffrey","T1 Post ax")
            ENRTY(1 ,"Lebowski^Jeffrey","GRASP ax_YP8")
            ENRTY(2 ,"Lebowski^Jeffrey","T2 HASTE cor")
            ENRTY(3 ,"Lebowski^Jeffrey","AdjCoilSens")
            ENRTY(4 ,"Seavers^Colt",    "T1 Post ax")
            ENRTY(5 ,"Seavers^Colt",    "GRASP ax_YB")
            ENRTY(6 ,"Seavers^Colt",    "TSE T2 FS ax")
            ENRTY(7 ,"Seavers^Colt",    "AdjCoilSens")
            ENRTY(8 ,"Urkel^Steve",     "HASTE cor")
            ENRTY(9 ,"Urkel^Steve",     "GRASP")
            ENRTY(10,"Urkel^Steve",     "AdjCoilSens")
            ENRTY(11,"Knight^Michael",  "T1 Post ax")
            ENRTY(12,"Knight^Michael",  "GRASP ax_YP8")
            ENRTY(13,"Knight^Michael",  "T2 HASTE cor")
            ENRTY(14,"Knight^Michael",  "AdjCoilSens")
            ENRTY(15,"MacGyver^Angus",  "DWI ax")
            ENRTY(16,"MacGyver^Angus",  "T1 GRASP_YH")
            ENRTY(17,"MacGyver^Angus",  "HASTE sag")
            ENRTY(18,"MacGyver^Angus",  "T1 pre tra")
            ENRTY(19,"MacGyver^Angus",  "AdjCoilSens")
            ENRTY(20,"Tanner^Willie",   "HASTE cor")
            ENRTY(21,"Tanner^Willie",   "GRASP_YSM")
            ENRTY(22,"Tanner^Willie",   "AdjCoilSens")

            printf (fileLine,
              fileIDOffset-i,
              measIDOffset-i,
              protName,
              patName,
              status,
              (uint64_t) filesize,
              (uint64_t) filesize,
              cTimeString,
              fTimeString);
            printf("\n");
        }
    }

    if (mode==2)
    {
        FILE * pFile;
        pFile = fopen (filename,"w");
        if (pFile!=NULL)
        {
            for (int j=0; j<filesize; j++)
            {
                fputc ('X',pFile);
            }
            fclose (pFile);
        }

        printf("MrParcThreadPrioImpl singleton created without singleton lock.\n");
        printf("MrParcThreadPrioImpl singleton created without singleton lock.\n");
        printf("  10%% copied  30%% copied  50%% copied  70%% copied 100%% copied\n");
        printf("Copied measurement data to file test.dat.");
        printf("\n");
    }

    if (mode==0)
    {
        printf("RAIDSimulator: Invalid arguments.\n");
        for (int i=0; i<argc; i++)
        {
            printf("%s|",argv[i]);
        }
        printf("\n");
    }

    return 0;
}

