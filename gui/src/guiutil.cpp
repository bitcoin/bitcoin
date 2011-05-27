#include "guiutil.h"

#include <QDateTime>

QString DateTimeStr(qint64 nTime)
{
    QDateTime date = QDateTime::fromMSecsSinceEpoch(nTime*1000);
    return date.toString(Qt::DefaultLocaleShortDate) + QString(" ") + date.toString("hh:mm");
}
