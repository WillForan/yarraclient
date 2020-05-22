#ifndef RDS_DEBUGWINDOW_H
#define RDS_DEBUGWINDOW_H

#include <QDialog>

namespace Ui {
    class rdsDebugWindow;
}


class rdsDebugWindow : public QDialog
{
    Q_OBJECT

public:
    explicit rdsDebugWindow(QWidget *parent = 0);
    ~rdsDebugWindow();

private slots:
    void on_setDebugButton_clicked();
    void on_testAnonymizerButton_clicked();
    void on_saveRaidOutputButton_clicked();
    void on_saveRaidListButton_clicked();

    void on_testFileButton_clicked();
    void on_parserTestButton_clicked();

private:
    Ui::rdsDebugWindow *ui;
};


#endif // RDS_DEBUGWINDOW_H


