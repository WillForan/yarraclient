#ifndef RDSCONFIGURATIONWINDOW_H
#define RDSCONFIGURATIONWINDOW_H

#include <QDialog>

#include "rds_configuration.h"


namespace Ui {
    class rdsConfigurationWindow;
}

class rdsConfigurationWindow : public QDialog
{
    Q_OBJECT

public:
    explicit rdsConfigurationWindow(QWidget *parent = 0);
    ~rdsConfigurationWindow();

    static bool checkAccessPassword();

    void readConfiguration();
    void storeConfiguration();

public slots:
    void saveSettings();
    void callUpdateModeChanged(int index);

    void callAddProt();
    void callRemoveProt();
    void callUpdateProt();
    void callShowProt();

    void callLogServerTestConnection();

protected:
    void closeEvent(QCloseEvent *event);

    void updateProtocolList();


private slots:
    void on_protListWidget_currentRowChanged(int currentRow);
    void on_protNameEdit_textEdited(const QString &arg1);
    void on_protFilterEdit_textEdited(const QString &arg1);

    void on_protAdjustCheckbox_toggled(bool checked);

    void on_protAnonymizeCheckbox_toggled(bool checked);

    void on_networkModeCombobox_currentIndexChanged(int index);

    void on_networkFilePathButton_clicked();

    void on_protSmallFilesCheckbox_toggled(bool checked);

private:
    Ui::rdsConfigurationWindow *ui;

    rdsConfiguration config;

};

#endif // RDSCONFIGURATIONWINDOW_H
