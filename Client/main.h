#ifndef MAIN_H
#define MAIN_H

#include <QApplication>
#include "qtsingleapplication.h"

class QT_QTSINGLEAPPLICATION_EXPORT rdsApplication : public QtSingleApplication
{
    Q_OBJECT
public:
    rdsApplication(int &argc, char **argv, bool GUIenabled = true);

public Q_SLOTS:
    void respond(const QString &message);
};


#endif // MAIN_H
