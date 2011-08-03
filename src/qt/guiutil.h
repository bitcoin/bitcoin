#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QString>

QT_BEGIN_NAMESPACE
class QFont;
class QLineEdit;
class QWidget;
class QDateTime;
QT_END_NAMESPACE

class GUIUtil
{
public:
    static QString DateTimeStr(qint64 nTime);
    static QString DateTimeStr(const QDateTime &datetime);

    // Render bitcoin addresses in monospace font
    static QFont bitcoinAddressFont();

    static void setupAddressWidget(QLineEdit *widget, QWidget *parent);

    static void setupAmountWidget(QLineEdit *widget, QWidget *parent);
};

#endif // GUIUTIL_H
