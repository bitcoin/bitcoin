#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QString>
#include <QFont>

QString DateTimeStr(qint64 nTime);
/* Render bitcoin addresses in monospace font */
QFont bitcoinAddressFont();

#endif // GUIUTIL_H
