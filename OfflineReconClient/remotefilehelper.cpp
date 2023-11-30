#include "remotefilehelper.h"

#include "../Client/rds_exechelper.h"
#include "../Client/rds_global.h"
#include "ort_global.h"

remoteFileHelper::remoteFileHelper(QObject *parent) : QObject(parent)
{

}

void remoteFileHelper::init(QString server, enum remoteFileHelper::connectionType type, QString hostKey) {
    if (type == remoteFileHelper::connectionType::FTP) {
        serverURI = "ftp://"+server;
    } else if (type == remoteFileHelper::connectionType::SFTP) {
        serverURI = "sftp://"+server;
    }
    connectionType = type;
    if (hostKey.isEmpty()) {
        hostkey = "acceptnew";
    } else {
        hostkey = hostKey;
    }
    winSCPPath = QDir(qApp->applicationDirPath()).filePath("WinSCP.com");
    RTI->log("Set server URI to " + server);
}

bool remoteFileHelper::testConnection() {
    if (serverURI.isEmpty()) {
        RTI->log("Server is not configured!");
        return false;
    }
    QStringList output;
    exec.setTimeout(ORT_CONNECT_TIMEOUT);
    bool success = runServerOperations(QStringList{}, output);
    if (!success) return false;
    for ( const QString& i : qAsConst(output))
    {
        if (i.contains("Session started")) {
            return true;
        }
    }
    return false;
}

bool remoteFileHelper::exists(QString path) {
    QStringList output;
    return runServerOperations(QStringList("stat "+path),output);
}

long int remoteFileHelper::size(QString path) {
    QStringList output;
    bool success = runServerOperations(QStringList("stat "+path),output);
    if (!success) {
        return -1;
    }
    QString stat_string = output.back();
    QString size_string = stat_string.split(" ",QString::SkipEmptyParts).at(2);
    bool ok;
    int size_int = size_string.toLong(&ok);
    if (ok) return size_int;
    return -1;
}

bool remoteFileHelper::get(QString path, QDir dest) {
   QStringList output;
   exec.setTimeout(RDS_COPY_TIMEOUT);
   return runServerOperation("get "+path+" "+QDir::toNativeSeparators(dest.absolutePath())+"\\", output);
}

bool remoteFileHelper::put(QString path) {
   QStringList output;
   exec.setTimeout(RDS_COPY_TIMEOUT);
   return runServerOperation("put "+QDir::toNativeSeparators(path)+" ./", output);
}


//bool remoteFileHelper::read(QString path, QString& output) {
//   QStringList lines;
//   exec.setTimeout(RDS_COPY_TIMEOUT);
//   bool success = runServerOperation("get "+path+" -", lines);
//   if (!success) return false;
//   output = lines.join("");
//   return true;
//}


bool remoteFileHelper::runServerOperation(QString operation, QStringList &output) {
    return runServerOperations(QStringList(operation), output);
}

bool remoteFileHelper::runServerOperations(QStringList operations, QStringList &output) {
    QString openCommand = "open " + serverURI;
    if (connectionType == remoteFileHelper::SFTP) {
        openCommand += " -hostkey=\"" + hostkey + "\"";
    }
    QStringList allOperations = QStringList(openCommand) + operations;
    return callWinSCP(allOperations, output);
}

bool remoteFileHelper::callWinSCP(QStringList commands, QStringList &output) {
    QString cmdString = winSCPPath +
            " /ini=nul /command" +
            " \"" + commands.join("\" \"") +"\""+
            " \"exit\"";
    RTI->log(cmdString);
    exec.run(cmdString);
    output = exec.output;
    RTI->log("Exit code: " + QString::number(exec.getExitCode()));
    RTI->log("Exec output: ");
    RTI->log(output.join("; "));

    if (exec.getExitCode()==0){
        return true;
    } else {
        return false;
    }
}
