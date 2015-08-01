#include "ort_confirmationdialog.h"
#include "ui_ort_confirmationdialog.h"

#include "ort_global.h"
#include "ort_configuration.h"
#include "../Client/rds_global.h"
#include <QDesktopWidget>
#include <QtWidgets>


ortConfirmationDialog::ortConfirmationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ortConfirmationDialog)
{
    ui->setupUi(this);

    setWindowIcon(ORT_ICON);
    setWindowTitle("Confirm Reconstruction Task");

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    confirmed=false;
    requiresACC=false;
    requiresParam=false;

    accessionHeight=ui->accessionFrame->maximumHeight()+3+8;
    paramHeight=ui->paramFrame->maximumHeight()+3+8;
    expandedHeight=maximumHeight();
    int newHeight=expandedHeight-accessionHeight-paramHeight;

    ui->accessionFrame->setVisible(false);
    ui->accessionLine->setVisible(false);
    ui->paramFrame->setVisible(false);
    ui->paramLine->setVisible(false);

    // Install event hooks from a helper class to enforce that
    // the cursor is always set to the first character when focusing
    // line edit controls with input mask.
    lineEditHelper.installOn(ui->accEdit);
    lineEditHelper.installOn(ui->paramEdit);

    // Adjust the height of the dialog after hiding the ACC section and center it.
    setMaximumHeight(newHeight);
    setMinimumHeight(newHeight);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(size().width(), newHeight), qApp->desktop()->availableGeometry()));

    configInstance=0;
}

ortConfirmationDialog::~ortConfirmationDialog()
{
    delete ui;
}


void ortConfirmationDialog::setConfigInstance(ortConfiguration* instance)
{
    configInstance=instance;
}


void ortConfirmationDialog::setACCRequired()
{
    requiresACC=true;

    ui->confirmButton->setEnabled(false);
    ui->accessionFrame->setVisible(true);
    ui->accessionLine->setVisible(true);
}


void ortConfirmationDialog::setParamRequired(QString label, QString description, int defaultValue, int minValue, int maxValue)
{
    requiresParam=true;

    paramDefault=defaultValue;
    paramMin=minValue;
    paramMax=maxValue;

    ui->paramLabel->setText(label+":");
    ui->paramEdit->setText(QString::number(defaultValue));
    ui->paramEdit->setCursorPosition(0);

    if (description!="")
    {
        ui->paramDescription->setText(description);
    }
    else
    {
        QString defDescription="The mode requires that you enter the following parameter (range: " +QString::number(paramMin)+ " - " +QString::number(paramMax)+ ").";
        ui->paramDescription->setText(defDescription);
    }

    ui->paramFrame->setVisible(true);
    ui->paramLine->setVisible(true);
}


void ortConfirmationDialog::updateDialogHeight()
{
    // Adjust the height of the dialog for the ACC + param section

    int newHeight=expandedHeight;

    if (!requiresACC)
    {
        newHeight-=accessionHeight;
    }
    if (!requiresParam)
    {
        newHeight-=paramHeight;
    }

    setMaximumHeight(newHeight);
    setMinimumHeight(newHeight);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(size().width(), newHeight), qApp->desktop()->availableGeometry()));
}


void ortConfirmationDialog::setPatientInformation(QString patientInfo)
{
    ui->patientInformationLabel->setText(patientInfo);
}


QString ortConfirmationDialog::getEnteredACC()
{
    return ui->accEdit->text();
}


QString ortConfirmationDialog::getEnteredMail()
{
    QString mailRecipients=ui->mailEdit->text();
    mailRecipients.remove(" ");
    mailRecipients.replace(";",",");

    return mailRecipients;
}


int ortConfirmationDialog::getEnteredParam()
{
    int enteredValue=paramDefault;

    if (ui->paramEdit->text()!="")
    {
        enteredValue=ui->paramEdit->text().toInt();
    }

    if (enteredValue>paramMax)
    {
        enteredValue=paramMax;
    }
    if (enteredValue<paramMin)
    {
        enteredValue=paramMin;
    }

    return enteredValue;
}


void ortConfirmationDialog::on_confirmButton_clicked()
{
    confirmed=true;
    close();
}


void ortConfirmationDialog::on_cancelButton_clicked()
{
    confirmed=false;
    close();
}


void ortConfirmationDialog::on_accEdit_textEdited(const QString &arg1)
{
    if (requiresACC)
    {
        if (arg1.length() > 0)
        {
            ui->confirmButton->setEnabled(true);
        }
        else
        {
            ui->confirmButton->setEnabled(false);
        }
    }
}



void ortConfirmationDialog::on_mailEdit_customContextMenuRequested(const QPoint &pos)
{
    if (configInstance==0)
    {
        return;
    }

    QMenu infoMenu(this);
    infoMenu.addAction("Insert @ symbol", this, SLOT(insertAtChar()));

    if (configInstance->ortMailPresets.count()>0)
    {
        infoMenu.addSeparator();

        for (int i=0; i<configInstance->ortMailPresets.count(); i++)
        {
            QString entry=configInstance->ortMailPresets.at(i);
            infoMenu.addAction(entry, this, SLOT(insertMailAddress()))->setProperty("email",entry);
        }
    }

    infoMenu.exec(ui->mailEdit->mapToGlobal(pos));
}


void ortConfirmationDialog::insertAtChar()
{
    ui->mailEdit->setText(ui->mailEdit->text()+"@");
}

void ortConfirmationDialog::insertMailAddress()
{
    ui->mailEdit->setText(ui->mailEdit->text()+sender()->property("email").toString());
}


