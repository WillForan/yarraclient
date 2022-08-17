#ifndef YDHELPER_H
#define YDHELPER_H

#include <QtCore>


class ydHelper
{
public:
    ydHelper();

    static bool pingServer(QString ipAddress);
    static QString extractIP(QString netUseCommand);

};


#endif // YDHELPER_H
