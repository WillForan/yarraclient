#include "yca_mainwindow.h"
#include "main.h"
#include "yca_global.h"

#include <QApplication>
#include <QWidget>
#include <QMessageBox>
#include <QStyleFactory>

#include <iostream>


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

    std::cout << "YarraCloud Agent v" << YCA_VERSION << std::endl;

    // If the application is already running, send a message instead of launching
    // another instance
    if (a.isRunning())
    {
        std::cout << "Running YCA instance found" << std::endl;
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
        std::cout << "Starting new instance" << std::endl;

        if (argc>1)
        {
            if (QString(argv[1])=="submit")
            {
                std::cout << "Triggering job submission" << std::endl;
                triggerSubmission=true;
            }
            if (QString(argv[1])=="show")
            {
                std::cout << "Triggering window display" << std::endl;
                triggerShow=true;
            }
        }

    }

    // Set color scheme
    /*
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette newPalette=QPalette(QColor(240,240,240),QColor(240,240,240));
    newPalette.setColor(QPalette::Highlight, QColor(247,176,44));
    //newPalette.setColor(QPalette::Highlight, QColor(192,152,95));
    //newPalette.setColor(QPalette::Highlight, QColor(88,15,139));
    qApp->setPalette(newPalette);
    */

    // Set color scheme
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette palette=QPalette(QColor(240,240,240),QColor(240,240,240));

    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     QColor(127, 127, 127));
    palette.setColor(QPalette::Base, QColor(42, 42, 42));
    palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, QColor(53, 53, 53));
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    palette.setColor(QPalette::Dark, QColor(35, 35, 35));
    palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     QColor(127, 127, 127));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));

    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     QColor(127, 127, 127));

    palette.setColor(QPalette::Highlight, QColor(247,176,44));
    qApp->setPalette(palette);



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

    std::cout << "After constructor, prior to window execution" << std::endl;
    a.exec();
    a.setActivationWindow(0, false);

    return 0;
}
