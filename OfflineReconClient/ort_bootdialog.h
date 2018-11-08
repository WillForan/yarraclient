#ifndef ORT_BOOTDIALOG_H
#define ORT_BOOTDIALOG_H

#include <QDialog>

namespace Ui {
class ortBootDialog;
}

class ortBootDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ortBootDialog(QWidget *parent = 0);
    ~ortBootDialog();

    void setFallbacktext();
    void setText(QString text);

private:
    Ui::ortBootDialog *ui;
};

#endif // ORT_BOOTDIALOG_H
