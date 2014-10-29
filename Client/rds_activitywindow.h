#ifndef RDS_ACTIVITYWINDOW_H
#define RDS_ACTIVITYWINDOW_H

#include <QDialog>

namespace Ui {
    class rdsActivityWindow;
}


class rdsActivityWindow : public QDialog
{
    Q_OBJECT

public:
    explicit rdsActivityWindow(QWidget *parent = 0);
    ~rdsActivityWindow();

public slots:
    void callPostponement();

private:
    Ui::rdsActivityWindow *ui;
};



#endif // RDS_ACTIVITYWINDOW_H


