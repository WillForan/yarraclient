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
    palette.setColor(QPalette::Highlight, QColor(224,165,38));
    palette.setColor(QPalette::Highlight, QColor(0,0,0));

    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     QColor(127, 127, 127));

    palette.setColor(QPalette::Highlight, QColor(255,106,19));
    qApp->setPalette(palette);

    QFont defaultFont = QApplication::font();
    defaultFont.setPointSize(defaultFont.pointSize()+1);
    a.setFont(defaultFont);

    if (a.isRunning())
    {
        // Shutdown this instance. Don't show error message to avoid
        // disturbing popups when using the client via YarraLink.
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
