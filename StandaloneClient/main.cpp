#include "sac_mainwindow.h"
#include <QtCore>
#include <QtGUI>
#include <QApplication>
#include <QStyleFactory>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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

        sacMainWindow* w=new sacMainWindow();
        w->show();
        returnValue=a.exec();
        restartApp=w->restartApp;
        delete w;
        w=0;
    }

    return returnValue;
}
