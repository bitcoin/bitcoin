#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QString>

QT_BEGIN_NAMESPACE
class QFont;
class QLineEdit;
class QWidget;
QT_END_NAMESPACE

class GUIUtil
{
public:
    static QString DateTimeStr(qint64 nTime);

    // Render bitcoin addresses in monospace font
    static QFont bitcoinAddressFont();

    static void setupAddressWidget(QLineEdit *widget, QWidget *parent);

    static void setupAmountWidget(QLineEdit *widget, QWidget *parent);

    // Convenience wrapper around ParseMoney that takes QString
    static bool parseMoney(const QString &amount, qint64 *val_out);

    // Convenience wrapper around FormatMoney that returns QString
    static QString formatMoney(qint64 amount, bool plussign=false);
};

#endif // GUIUTIL_H
