#ifndef SAC_CONFIGURATIONDIALOG_H
#define SAC_CONFIGURATIONDIALOG_H

#include <QDialog>

namespace Ui {
class sacConfigurationDialog;
}

class sacMainWindow;

class sacConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit sacConfigurationDialog(QWidget *parent = 0);
    ~sacConfigurationDialog();

    void prepare(sacMainWindow* mainWindowPtr);

    bool closeMainWindow;

private slots:
    void on_cancelButton_clicked();
    void on_saveButton_clicked();
    void on_cloudCheckbox_clicked(bool checked);
    void on_cloudCredentialsButton_clicked();

    void on_cloudConnectionButton_clicked();

    void on_cloudProxyButton_clicked();

private:
    Ui::sacConfigurationDialog *ui;

    sacMainWindow* mainWindow;

    void updateCloudCredentialStatus();
    void updateProxyStatus();

};

#endif // SAC_CONFIGURATIONDIALOG_H
