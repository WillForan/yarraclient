#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QMainWindow>
#include <rds_updater.h>

QT_BEGIN_NAMESPACE
namespace Ui { class UpdateWindow; }
QT_END_NAMESPACE

class UpdateWindow : public QMainWindow
{
    Q_OBJECT

public:
    UpdateWindow(QWidget *parent = nullptr);
    ~UpdateWindow();

private slots:
    void on_updateVersionsWidget_currentRowChanged(int currentRow);

    void on_doUpdateButton_clicked();

    void on_updateModeSet_stateChanged(int arg1);

private:
    Ui::UpdateWindow *ui;
    rdsUpdater updater;
};
#endif // UPDATEWINDOW_H
