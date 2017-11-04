#ifndef SAC_NETWORK_H
#define SAC_NETWORK_H

#include <QtCore>


class sacNetwork: public QObject
{
    Q_OBJECT

public:
    sacNetwork();

    QString connectCmd;
    QString serverPath;
    QString disconnectCmd;
    QString systemName;

    QString defaultNotification;
    QString preferredMode;

    QString appPath;

    QDir serverDir;

    QString copyErrorMsg;
    bool showConfigurationAfterError;

    bool readConfiguration();
    void writeConfiguration();

    bool openConnection(bool isConsole);
    void closeConnection();

    bool copyMeasurementFile(QString sourceFile, QString targetFile);

};


#endif // SAC_NETWORK_H
