#include "rds_checksum.h"

#define RDS_CHECKSUM_BLOCKSIZE 1048576
//#define RDS_CHECKSUM_BLOCKSIZE 8192


rdsChecksum::rdsChecksum()
{
}


QString rdsChecksum::getChecksum(QString filename)
{
    QCryptographicHash crypto(QCryptographicHash::Md5);

    QFile file(filename);

    file.open(QFile::ReadOnly);
    while(!file.atEnd()){
        crypto.addData(file.read(RDS_CHECKSUM_BLOCKSIZE));
    }

    QByteArray hash = crypto.result();

    return QString(hash.toHex());
}
