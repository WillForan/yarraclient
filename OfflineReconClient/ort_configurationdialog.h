#ifndef ORT_CONFIGURATIONDIALOG_H
#define ORT_CONFIGURATIONDIALOG_H

#include <QDialog>

namespace Ui {
class ortConfigurationDialog;
}

class ortConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ortConfigurationDialog(QWidget *parent = 0);
    ~ortConfigurationDialog();

    static bool executeDialog();

    bool requiresRestart;

    void readSettings();
    void writeSettings();

    bool checkAccessPassword();

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::ortConfigurationDialog *ui;
};

#endif // ORT_CONFIGURATIONDIALOG_H
