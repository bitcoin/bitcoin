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

bool GUIUtil::parseBitcoinURL(const QUrl *url, SendCoinsRecipient *out)
{
    if(url->scheme() != QString("bitcoin"))
        return false;

    SendCoinsRecipient rv;
    rv.address = url->path();
    rv.label = url->queryItemValue("label");

    QString amount = url->queryItemValue("amount");
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
