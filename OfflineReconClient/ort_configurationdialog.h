#ifndef ORT_CONFIGURATIONDIALOG_H
#define ORT_CONFIGURATIONDIALOG_H

#include <QDialog>

#include "../CloudTools/yct_configuration.h"
#include "../CloudTools/yct_api.h"


namespace Ui {
class ortConfigurationDialog;
}

class ortConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ortConfigurationDialog(QWidget *parent=0);
    ~ortConfigurationDialog();

    static bool executeDialog();

    bool requiresRestart;

    void readSettings();
    void writeSettings();

    bool checkAccessPassword();

    yctConfiguration  cloudConfig;
    yctAPI            cloud;

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();

    void on_logServerTestButton_clicked();

    void on_cloudConnectionButton_clicked();
    void on_cloudCredentialsButton_clicked();
    void on_cloudCheckbox_clicked(bool checked);

    void on_cloudProxyButton_clicked();

    void on_serverTypeComboBox_currentIndexChanged(const QString &arg1);

private:
    Ui::ortConfigurationDialog *ui;

    void updateCloudCredentialStatus();
    void updateProxyStatus();

};

#endif // ORT_CONFIGURATIONDIALOG_H
