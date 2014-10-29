#ifndef ORT_COPYDIALOG_H
#define ORT_COPYDIALOG_H

#include <QDialog>

namespace Ui {
class ortCopyDialog;
}

class ortCopyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ortCopyDialog(QWidget *parent = 0);
    ~ortCopyDialog();

private:
    Ui::ortCopyDialog *ui;
};

#endif // ORT_COPYDIALOG_H
