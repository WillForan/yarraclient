#ifndef ORT_WAITDIALOG_H
#define ORT_WAITDIALOG_H

#include <QDialog>

namespace Ui {
class ortWaitDialog;
}

class ortWaitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ortWaitDialog(QWidget *parent = 0);
    ~ortWaitDialog();

private:
    Ui::ortWaitDialog *ui;
};

#endif // ORT_WAITDIALOG_H
