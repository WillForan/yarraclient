#include "remotefilehelper.h"

#include "../Client/rds_exechelper.h"
#include "../Client/rds_global.h"
#include "ort_global.h"

remoteFileHelper::remoteFileHelper(QObject *parent) : QObject(parent)
{
}

void remoteFileHelper::init(QString server, QString hostKey) {
    serverURI = server;
    if (server.startsWith("ftp://")) {
        connectionType = FTP;
    } else if (server.startsWith("sftp://")) {
        connectionType = SFTP;
    } else if (server.startsWith("scp://")) {
        connectionType = SCP;
    } else {
        connectionType = UNKNOWN;
    }
    if (hostKey.isEmpty()) {
        hostkey = "acceptnew";
    } else {
        hostkey = hostKey;
    }
    winSCPPath = QDir(qApp->applicationDirPath()).filePath("WinSCP.com");
    RTI->log("Set server URI to " + server);
    remoteBasePath = "/YarraServer";
}

bool remoteFileHelper::testConnection(QString& error) {
    if (serverURI.isEmpty()) {
        error = "The Yarra server URI has not been defined.\nPlease check the configuration.";
        return false;
    }
    QStringList output;
    exec.setTimeout(ORT_CONNECT_TIMEOUT);
    bool success = runServerOperations(QStringList{}, output);
    if (!success) {
        error = output.mid(output.count()-3,3).join("").trimmed();
        return false;
    }

    success = exists("");
    if (!success) {
        error = "Expected directory " + remoteBasePath + " missing.";
        return false;
    }
    return true;

}

bool remoteFileHelper::exists(QString path) {
    QStringList output;
    exec.setTimeout(ORT_CONNECT_TIMEOUT);
    return runServerOperations(QStringList("stat \"\"" + remoteBasePath + "/"+path+"\"\""),output);
}

bool remoteFileHelper::exists(QStringList paths) {
    QStringList output;
    exec.setTimeout(ORT_CONNECT_TIMEOUT);
    QStringList statCommands;
    for ( const QString& i : static_cast<const QStringList&>(paths)) {
        statCommands.append("stat \"\"" + remoteBasePath + "/"+i+"\"\"");
    }
    return runServerOperations(statCommands, output);
}

long int remoteFileHelper::size(QString path) {
    QStringList output;
    exec.setTimeout(ORT_CONNECT_TIMEOUT);
    bool success = runServerOperations(QStringList("stat \"\"" + remoteBasePath + "/"+path+"\"\""),output);
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

bool remoteFileHelper::get(QStringList paths, QDir dest) {
   QStringList output;
   exec.setTimeout(RDS_COPY_TIMEOUT);
   QStringList getCommands;
   for ( const QString& i : static_cast<const QStringList&>(paths)) {
       getCommands.append("get \"\"" + remoteBasePath + "/"+i+"\"\" \"\""+QDir::toNativeSeparators(dest.absolutePath())+"\\\"\"");
   }
   return runServerOperations(getCommands, output);
}

bool remoteFileHelper::get(QString path, QDir dest) {
   QStringList output;
   exec.setTimeout(RDS_COPY_TIMEOUT);
   return runServerOperation("get \"\"" + remoteBasePath + "/"+path+"\"\" \"\""+QDir::toNativeSeparators(dest.absolutePath())+"\\\"\"", output);
}

bool remoteFileHelper::put(QString path) {
   QStringList output;
   exec.setTimeout(RDS_COPY_TIMEOUT);
   return runServerOperation("put \"\""+QDir::toNativeSeparators(path)+"\"\" " + remoteBasePath + "/", output);
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
    if (connectionType == SFTP || connectionType == SCP) {
        if (hostkey == "acceptnew") {
            openCommand += " -hostkey=acceptnew";
        } else {
            openCommand += " -hostkey=\"\"" + hostkey + "\"\"";
        }
    }
    QStringList allOperations = QStringList(openCommand) + operations;
    return callWinSCP(allOperations, output);
}

bool remoteFileHelper::callWinSCP(QStringList commands, QStringList &output) {
    QString cmdString = "/ini=nul /command \"" + commands.join("\" \"") +"\""+
            " \"exit\"";
    RTI->log(cmdString);
//    QStringList allCommands = QStringList{"/ini=nul", "/command"} + commands + QStringList{"exit"};
//    RTI->log(winSCPPath+" \""+allCommands.join("\" \"")+"\"");
    exec.run(winSCPPath, cmdString);
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
