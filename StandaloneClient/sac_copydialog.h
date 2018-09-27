#ifndef SAC_COPYDIALOG_H
#define SAC_COPYDIALOG_H

#include <QDialog>

namespace Ui {
class sacCopyDialog;
}

class sacCopyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit sacCopyDialog(QWidget *parent = 0);
    ~sacCopyDialog();

    void setText(QString text);

private:
    Ui::sacCopyDialog *ui;
};

#endif // SAC_COPYDIALOG_H
