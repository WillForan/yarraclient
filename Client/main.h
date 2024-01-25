#ifndef MAIN_H
#define MAIN_H

#include <QtCore>
#include <QApplication>
#include "qtsingleapplication.h"
#include "rds_mailbox.h"

class QT_QTSINGLEAPPLICATION_EXPORT rdsApplication : public QtSingleApplication
{
    Q_OBJECT
public:
    rdsApplication(int &argc, char **argv, bool GUIenabled = true);
    Mailbox mailbox;
public Q_SLOTS:
    void respond(const QString &message);
};


#endif // MAIN_H
