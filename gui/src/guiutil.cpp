#include "guiutil.h"

#include <QDateTime>

QString DateTimeStr(qint64 nTime)
{
    QDateTime date = QDateTime::fromMSecsSinceEpoch(nTime*1000);
    return date.date().toString(Qt::SystemLocaleShortDate) + QString(" ") + date.toString("hh:mm");
}

QFont bitcoinAddressFont()
{
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    return font;
}
