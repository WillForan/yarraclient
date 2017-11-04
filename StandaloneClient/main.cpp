#include "sac_mainwindow.h"
#include <QtCore>
#include <QtGUI>
#include <QApplication>
#include <QStyleFactory>
#include "sac_batchdialog.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Parse command line options
    QCommandLineParser parser;
    parser.setApplicationDescription("SAC client");
    parser.addHelpOption();
    QCommandLineOption batchSubmitOption("b","batch file name","batch","");
    parser.addOption(batchSubmitOption);
    parser.process(a);
    bool batchSubmit = parser.isSet(batchSubmitOption);

    // Set color scheme
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette newPalette=QPalette(QColor(240,240,240),QColor(240,240,240));
    newPalette.setColor(QPalette::Highlight, QColor(67,176,42));
    qApp->setPalette(newPalette);

    bool restartApp=true;
    int returnValue=0;

    while (restartApp)
    {
        restartApp=false;

        sacMainWindow* w=new sacMainWindow(NULL,batchSubmit);
        if (!batchSubmit) {
            w->show();
            returnValue=a.exec();
            restartApp=w->restartApp;
            delete w;
            w=0;
        } else {
            if (!w->didStart) {
                qInfo() << "Startup error";
                return 1;
            }
            QStringList files_l{};
            QStringList modes_l{};
            qInfo() << "Reading batch file...";
            QString file = parser.value(batchSubmitOption);
            QString notify;
            TaskPriority priority;
            if (! w->readBatchFile(file,files_l, modes_l, notify, priority))
            {
                return 1;
            }

            qInfo() << "Done reading batch file.";
            w->submitBatch(files_l,modes_l,notify,priority);
        }
    }

    return returnValue;
}
