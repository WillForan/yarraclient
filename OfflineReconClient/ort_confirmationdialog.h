#ifndef ORT_CONFIRMATIONDIALOG_H
#define ORT_CONFIRMATIONDIALOG_H

#include <QtWidgets>
#include "ort_returnonfocus.h"

namespace Ui {
class ortConfirmationDialog;
}


class ortConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ortConfirmationDialog(QWidget *parent = 0);
    ~ortConfirmationDialog();

    void setPatientInformation(QString patientInfo);
    void setACCRequired();
    void setParamRequired(QString label, QString description, int defaultValue, int minValue, int maxValue);

    void updateDialogHeight();

    bool isACCRequired();
    bool isConfirmed();

    QString getEnteredACC();
    QString getEnteredMail();
    int     getEnteredParam();

private slots:
    void on_confirmButton_clicked();
    void on_cancelButton_clicked();
    void on_accEdit_textEdited(const QString &arg1);

    void on_mailEdit_customContextMenuRequested(const QPoint &pos);

    void insertAtChar();
    void insertMailAddress();

private:
    Ui::ortConfirmationDialog *ui;

    bool confirmed;
    bool requiresACC;
    bool requiresParam;

    int accessionHeight;
    int paramHeight;
    int expandedHeight;

    int paramDefault;
    int paramMin;
    int paramMax;

    ortReturnOnFocus lineEditHelper;

};


inline bool ortConfirmationDialog::isConfirmed()
{
    return confirmed;
}


inline bool ortConfirmationDialog::isACCRequired()
{
    return requiresACC;
}





#endif // ORT_CONFIRMATIONDIALOG_H
