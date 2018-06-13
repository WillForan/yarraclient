#ifndef MAIN_H
#define MAIN_H

#include <QApplication>
#include "QtSingleApplication.h"

class QT_QTSINGLEAPPLICATION_EXPORT ycaApplication : public QtSingleApplication
{
    Q_OBJECT
public:
    ycaApplication(int &argc, char **argv, bool GUIenabled = true);

public Q_SLOTS:
    void respond(const QString &message);
};


#endif // MAIN_H
