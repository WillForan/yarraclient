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

    void setAnim(bool value);
    void clearError();
    void setError();

    void showStartupCommandsOption();
    void showCloudWindowOption();

private:
    Ui::rdsIconWindow* ui;

    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    bool    animRunning;
    bool    error;
    QMovie* anim;
    bool    showStartupCommandsEntry;
    bool    showCloudWindowEntry;

private slots:
    void showStatusWindow();
    void showCloudAgent();
    void runStartupCommands();

};


#endif // RDS_ICONWINDOW_H

