#include "ort_mainwindow.h"
#include <QApplication>
#include <QWidget>
#include <QtGui>
#include <QtWidgets>
#include <QStyleFactory>

#include "QtSingleApplication.h"
#include "../Client/rds_global.h"

int main(int argc, char *argv[])
{
    QtSingleApplication a(argc, argv);
    Q_INIT_RESOURCE(ort);

    // Set color scheme
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette newPalette=QPalette(QColor(240,240,240),QColor(240,240,240));
    newPalette.setColor(QPalette::Highlight, QColor(88,15,139));
    qApp->setPalette(newPalette);

    if (a.isRunning())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Yarra ORT already running");
        msgBox.setText("The ORT client is already running.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(RDS_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        // Shutdown this instance
        return 0;
    }

    // Re-use some of the funtionality implemented by the RDS client.
    // Thus, create a limited instance of the RDS runtime environment.
    RTI->prepare();

    if (RTI->isInvalidEnvironment())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Invalid Runtime Environment");
        msgBox.setText("This program can only be used on the host computer \n of a Siemens MAGNETOM system.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowIcon(RDS_ICON);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        return 0;
    }

    if (!RTI->prepareEnvironment())
    {
        return 0;
    }


    ortMainWindow w;
    w.show();

    return a.exec();
}
