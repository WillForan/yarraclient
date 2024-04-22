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

    bool cloudSupportEnabled;

    QString copyErrorMsg;
    bool showConfigurationAfterError;

    bool readConfiguration();
    void writeConfiguration();

    bool openConnection(bool isConsole);
    void closeConnection();
    bool checkConnection();

    virtual QSettings* readModelist(QString& error);

    bool copyMeasurementFile(QString sourceFile, QString targetFile);
    bool fileExistsOnServer(QString filename);

};


#endif // SAC_NETWORK_H
