#include "yd_test_systeminfo.h"
#include "yd_global.h"

#include "../Client/rds_exechelper.h"


ydTestSysteminfo::ydTestSysteminfo() : ydTest()
{
}


QString ydTestSysteminfo::getName()
{
    return "System Info";
}


QString ydTestSysteminfo::getDescription()
{
    return "collect general system information";
}


bool ydTestSysteminfo::run(QString& issues, QString& results)
{
    // Get general system information
    YD_RESULT_STARTSECTION
    YD_ADDRESULT("Operating system: " + QSysInfo::prettyProductName() + ", kernel: " + QSysInfo::kernelVersion())
    YD_ADDRESULT_LINE("Hostname: " + QSysInfo::machineHostName())
    YD_RESULT_ENDSECTION

    // Check drives and available disk space
    YD_RESULT_STARTSECTION
    YD_ADDRESULT("Available drives:")
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes())
    {
        if (storage.isValid() && storage.isReady()) {

            YD_ADDRESULT_LINE("------");
            YD_ADDRESULT_LINE("Drive: " + storage.rootPath() + "  (volume name: "+storage.name()+")");
            YD_ADDRESULT_LINE("Filesystem: " + storage.fileSystemType() + " (" + (storage.isReadOnly()? "read-only" : "read/write" ) + ")");
            YD_ADDRESULT_LINE("Size: " + QString::number(storage.bytesTotal()/1000/1000) + " MB");
            if (storage.bytesTotal()>0)
            {
                int percentageFree = int(100*storage.bytesAvailable()/storage.bytesTotal());
                YD_ADDRESULT_LINE("Free: " + QString::number(storage.bytesAvailable()/1000/1000) + " MB ("+QString::number(percentageFree)+" %)");
                if ((!storage.isReadOnly()) && ((percentageFree <= 10) || (storage.bytesAvailable()/1000/1000 < 20000)))
                {
                    YD_ADDRESULT_COLORLINE("Available space on drive is critically low", YD_WARNING);
                    YD_ADDISSUE("Available space on drive "+ storage.rootPath() +" critically low",YD_WARNING);
                }
            }
        }
    }
    YD_RESULT_ENDSECTION

    // Check mapped network drives
    {
        YD_RESULT_STARTSECTION
        YD_ADDRESULT("Network shares (net use):")
        rdsExecHelper execHelper;
        QString command = "net use";
        execHelper.setCommand(command);
        execHelper.run();
        YD_ADDRESULT_LINE("[--- Capture Begin ---]");
        for (int i=0; i<execHelper.output.length(); i++)
        {
            YD_ADDRESULT_LINE(execHelper.output.at(i));
        }
        YD_ADDRESULT_LINE("[--- Capture End ---]");
        YD_RESULT_ENDSECTION
    }

    // Check network configuration / DNS settings
    {
        YD_RESULT_STARTSECTION
        YD_ADDRESULT("Network configuration (ipconfig):")
        rdsExecHelper execHelper;
        QString command = "ipconfig /all";
        execHelper.setCommand(command);
        execHelper.run();
        YD_ADDRESULT_LINE("[--- Capture Begin ---]");
        for (int i=0; i<execHelper.output.length(); i++)
        {
            YD_ADDRESULT_LINE(execHelper.output.at(i));
        }
        YD_ADDRESULT_LINE("[--- Capture End ---]");
        YD_RESULT_ENDSECTION
    }

    return true;
}
