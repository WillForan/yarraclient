#ifndef YARRA_UPDATER_H
#define YARRA_UPDATER_H

#undef NETLOGGER_LOG
#define NETLOGGER_LOG(x) qDebug().noquote() << x

#include <QtCore/QLibrary>
#include <QtCore/qt_windows.h>

#endif // YARRA_UPDATER_H
