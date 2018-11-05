#ifndef YCA_DETAILSDIALOG_H
#define YCA_DETAILSDIALOG_H

#include <QDialog>

#include "yca_task.h"


namespace Ui {
class ycaDetailsDialog;
}


class ycaDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ycaDetailsDialog(QWidget *parent = 0);
    ~ycaDetailsDialog();

    void setTaskDetails(ycaTask* task);


private slots:
    void on_closeButton_clicked();

private:
    Ui::ycaDetailsDialog *ui;
};


#endif // YCA_DETAILSDIALOG_H
