#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QString>

QT_BEGIN_NAMESPACE
class QFont;
class QLineEdit;
class QWidget;
class QDateTime;
class QUrl;
class QAbstractItemView;
QT_END_NAMESPACE
class SendCoinsRecipient;

/** Static utility functions used by the Bitcoin Qt UI.
 */
class GUIUtil
{
public:
    // Create human-readable string from date
    static QString dateTimeStr(qint64 nTime);
    static QString dateTimeStr(const QDateTime &datetime);

    // Render bitcoin addresses in monospace font
    static QFont bitcoinAddressFont();

    // Set up widgets for address and amounts
    static void setupAddressWidget(QLineEdit *widget, QWidget *parent);
    static void setupAmountWidget(QLineEdit *widget, QWidget *parent);

    // Parse "bitcoin:" URL into recipient object, return true on succesful parsing
    // See Bitcoin URL definition discussion here: https://bitcointalk.org/index.php?topic=33490.0
    static bool parseBitcoinURL(const QUrl *url, SendCoinsRecipient *out);

    // HTML escaping for rich text controls
    static QString HtmlEscape(const QString& str, bool fMultiLine=false);
    static QString HtmlEscape(const std::string& str, bool fMultiLine=false);

    /** Copy a field of the currently selected entry of a view to the clipboard. Does nothing if nothing
        is selected.
       @param[in] column  Data column to extract from the model
       @param[in] role    Data role to extract from the model
       @see  TransactionView::copyLabel, TransactionView::copyAmount, TransactionView::copyAddress
     */
    static void copyEntryData(QAbstractItemView *view, int column, int role=Qt::EditRole);

};

#endif // GUIUTIL_H
