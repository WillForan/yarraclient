#ifndef RDS_MAILBOXWINDOW_H
#define RDS_MAILBOXWINDOW_H

#include <QDialog>
#include <QAbstractButton>
#include <qevent.h>
#include <QScreen>


namespace Ui {
class rdsMailboxWindow;
}


class rdsMailboxWindow : public QDialog
{
    Q_OBJECT

public:
    explicit rdsMailboxWindow(QWidget *parent = 0);
    ~rdsMailboxWindow();
    void setMessage(QString message);
    void setError(QString message);

signals:
    void closing(QString button);
private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
protected:
//    virtual void closeEvent( QCloseEvent* event) override;
    virtual void keyPressEvent( QKeyEvent* event) override;
private:
    QString buttonClicked;
    Ui::rdsMailboxWindow *ui;
};


#endif // RDS_MAILBOXWINDOW_H
