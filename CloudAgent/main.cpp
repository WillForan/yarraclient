#include "yca_mainwindow.h"
#include "main.h"

#include <QApplication>
#include <QWidget>
#include <QMessageBox>
#include <QStyleFactory>


ycaApplication::ycaApplication(int &argc, char **argv, bool GUIenabled)
    : QtSingleApplication(argc, argv, GUIenabled)
{
    connect(this, SIGNAL(messageReceived(const QString&)), this, SLOT(respond(const QString&)));
}


void ycaApplication::respond(const QString &message)
{
    if (activationWindow()==0)
    {
        return;
    }

    if (message=="show")
    {
        activateWindow();
        ((ycaMainWindow*) activationWindow())->showNormal();
    }

    if (message=="terminate")
    {
        ((ycaMainWindow*) activationWindow())->callShutDown(false);
    }

    if (message=="submit")
    {
        ((ycaMainWindow*) activationWindow())->callSubmit();
    }

    // TODO: Command for refreshing the configuraiton
}


int main(int argc, char *argv[])
{
    // NOTE: A slight modification has been introduced to QtSingleApplication
    //       in the .activateWindow() method. It was necessary to add a call
    //       to .show() because otherwise the operation window remains hidden.
    ycaApplication a(argc, argv);
    Q_INIT_RESOURCE(yca);

    bool triggerSubmission=false;
    bool triggerShow=false;

    // If the application is already running, send a message instead of launching
    // another instance
    if (a.isRunning())
    {
        QString msg="";

        if (argc>1)
        {
            msg=argv[1];
        }

        a.sendMessage(msg);

        // Shutdown this instance
        return 0;
    }
    else
    {
        if (argc>1)
        {
            if (QString(argv[1])=="submit")
            {
                triggerSubmission=true;
            }
            if (QString(argv[1])=="show")
            {
                triggerShow=true;
            }
        }

    }

    // Set color scheme
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette newPalette=QPalette(QColor(240,240,240),QColor(240,240,240));
    newPalette.setColor(QPalette::Highlight, QColor(247,176,44));
    //newPalette.setColor(QPalette::Highlight, QColor(192,152,95));
    //newPalette.setColor(QPalette::Highlight, QColor(88,15,139));
    qApp->setPalette(newPalette);

    ycaMainWindow w;
    a.setActivationWindow(&w, false);
    if ((triggerSubmission) && (!w.shuttingDown))
    {
        QTimer::singleShot(0, &w, SLOT(callSubmit()));
    }
    if ((triggerShow) && (!w.shuttingDown))
    {
        QTimer::singleShot(0, &w, SLOT(showNormal()));
    }
    a.exec();
    a.setActivationWindow(0, false);

    return 0;
}
