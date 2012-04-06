#include "guiutil.h"
#include "bitcoinaddressvalidator.h"
#include "walletmodel.h"
#include "bitcoinunits.h"

#include "headers.h"

#include <QString>
#include <QDateTime>
#include <QDoubleValidator>
#include <QFont>
#include <QLineEdit>
#include <QUrl>
#include <QFileDialog>
#include <QDesktopServices>

QString GUIUtil::dateTimeStr(qint64 nTime)
{
    return dateTimeStr(QDateTime::fromTime_t((qint32)nTime));
}

QString GUIUtil::dateTimeStr(const QDateTime &date)
{
    return date.date().toString(Qt::SystemLocaleShortDate) + QString(" ") + date.toString("hh:mm");
}

QFont GUIUtil::bitcoinAddressFont()
{
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    return font;
}

void GUIUtil::setupAddressWidget(QLineEdit *widget, QWidget *parent)
{
    widget->setMaxLength(BitcoinAddressValidator::MaxAddressLength);
    widget->setValidator(new BitcoinAddressValidator(parent));
    widget->setFont(bitcoinAddressFont());
}

void GUIUtil::setupAmountWidget(QLineEdit *widget, QWidget *parent)
{
    QDoubleValidator *amountValidator = new QDoubleValidator(parent);
    amountValidator->setDecimals(8);
    amountValidator->setBottom(0.0);
    widget->setValidator(amountValidator);
    widget->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
}

bool GUIUtil::parseBitcoinURI(const QUrl *uri, SendCoinsRecipient *out)
{
    if(uri->scheme() != QString("bitcoin"))
        return false;

    SendCoinsRecipient rv;
    rv.address = uri->path();
    rv.label = uri->queryItemValue("label");

    QString amount = uri->queryItemValue("amount");
    if(amount.isEmpty())
    {
        rv.amount = 0;
    }
    else // Amount is non-empty
    {
        if(!BitcoinUnits::parse(BitcoinUnits::BTC, amount, &rv.amount))
        {
            return false;
        }
    }
    if(out)
    {
        *out = rv;
    }
    return true;
}

bool GUIUtil::parseBitcoinURI(QString uri, SendCoinsRecipient *out)
{
    // Convert bitcoin:// to bitcoin:
    //
    //    Cannot handle this later, because bitcoin:// will cause Qt to see the part after // as host,
    //    which will lowercase it (and thus invalidate the address).
    if(uri.startsWith("bitcoin://"))
    {
        uri.replace(0, 10, "bitcoin:");
    }
    QUrl uriInstance(uri);
    return parseBitcoinURI(&uriInstance, out);
}

QString GUIUtil::getSaveFileName(QWidget *parent, const QString &caption,
                                 const QString &dir,
                                 const QString &filter,
                                 QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty()) // Default to user documents location
    {
        myDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    }
    else
    {
        myDir = dir;
    }
    QString result = QFileDialog::getSaveFileName(parent, caption, myDir, filter, &selectedFilter);

    /* Extract first suffix from filter pattern "Description (*.foo)" or "Description (*.foo *.bar ...) */
    QRegExp filter_re(".* \\(\\*\\.(.*)[ \\)]");
    QString selectedSuffix;
    if(filter_re.exactMatch(selectedFilter))
    {
        selectedSuffix = filter_re.cap(1);
    }

    /* Add suffix if needed */
    QFileInfo info(result);
    if(!result.isEmpty())
    {
        if(info.suffix().isEmpty() && !selectedSuffix.isEmpty())
        {
            /* No suffix specified, add selected suffix */
            if(!result.endsWith("."))
                result.append(".");
            result.append(selectedSuffix);
        }
    }

    /* Return selected suffix if asked to */
    if(selectedSuffixOut)
    {
        *selectedSuffixOut = selectedSuffix;
    }
    return result;
}

