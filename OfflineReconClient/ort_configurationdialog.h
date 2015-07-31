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

private:
    Ui::ortConfigurationDialog *ui;
};

#endif // ORT_CONFIGURATIONDIALOG_H
