#ifndef SAC_BOOTDIALOG_H
#define SAC_BOOTDIALOG_H

#include <QDialog>

namespace Ui {
class sacBootDialog;
}

class sacBootDialog : public QDialog
{
    Q_OBJECT

public:
    explicit sacBootDialog(QWidget *parent = 0);
    ~sacBootDialog();

private:
    Ui::sacBootDialog *ui;
};

#endif // SAC_BOOTDIALOG_H
