#ifndef RDS_ICONWINDOW_H
#define RDS_ICONWINDOW_H

#include <QDialog>

namespace Ui
{
    class rdsIconWindow;
}


class rdsIconWindow : public QDialog
{
    Q_OBJECT

public:
    explicit rdsIconWindow(QWidget *parent = 0);
    ~rdsIconWindow();

private:
    Ui::rdsIconWindow* ui;
};


#endif // RDS_ICONWINDOW_H

