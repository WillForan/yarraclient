#ifndef YCA_TRANSFERINDICATOR_H
#define YCA_TRANSFERINDICATOR_H

#include <QDialog>

namespace Ui {
class ycaTransferIndicator;
}

class ycaTransferIndicator : public QDialog
{
    Q_OBJECT

public:
    explicit ycaTransferIndicator(QWidget *parent = 0);
    ~ycaTransferIndicator();

    QMovie* anim;

    void showIndicator();
    void hideIndicator();

    void setMainDialog(QDialog* parent);

private:
    Ui::ycaTransferIndicator *ui;

    void mouseDoubleClickEvent(QMouseEvent *event);

    QDialog* mainDialog;

};

#endif // YCA_TRANSFERINDICATOR_H
