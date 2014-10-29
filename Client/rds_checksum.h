#ifndef RDS_CHECKSUM_H
#define RDS_CHECKSUM_H

#include <QtCore>

class rdsChecksum
{
public:
    rdsChecksum();

    static QString getChecksum(QString filename);
};

#endif // RDS_CHECKSUM_H
