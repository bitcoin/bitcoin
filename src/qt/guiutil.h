#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QString>

QT_BEGIN_NAMESPACE
class QFont;
class QLineEdit;
class QWidget;
class QDateTime;
class QUrl;
QT_END_NAMESPACE
class SendCoinsRecipient;

class GUIUtil
{
public:
    // Create human-readable string from date
    static QString DateTimeStr(qint64 nTime);
    static QString DateTimeStr(const QDateTime &datetime);

    // Render bitcoin addresses in monospace font
    static QFont bitcoinAddressFont();

    // Set up widgets for address and amounts
    static void setupAddressWidget(QLineEdit *widget, QWidget *parent);
    static void setupAmountWidget(QLineEdit *widget, QWidget *parent);

    // Parse "bitcoin:" URL into recipient object, return true on succesful parsing
    static bool parseBitcoinURL(const QUrl *url, SendCoinsRecipient *out);
};

#endif // GUIUTIL_H
