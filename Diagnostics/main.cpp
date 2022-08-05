#include <QtCore>
#include <QtGUI>
#include <QApplication>
#include <QStyleFactory>

#include "yd_mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet("QPushButton { padding: 8px; }");

    // Set color scheme
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette palette=QPalette(QColor(240,240,240),QColor(240,240,240));
    //palette.setColor(QPalette::Highlight, QColor(67,176,42));

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
    palette.setColor(QPalette::Highlight, QColor(224,165,38));
    palette.setColor(QPalette::Highlight, QColor(0,0,0));

    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     QColor(127, 127, 127));

    qApp->setPalette(palette);


    QFont defaultFont = QApplication::font();
    defaultFont.setPointSize(defaultFont.pointSize()+1);
    a.setFont(defaultFont);

    int returnValue=0;
    ydMainWindow* w=new ydMainWindow(NULL);
    w->show();
    returnValue=a.exec();
    delete w;
    w=0;

    return returnValue;
}
