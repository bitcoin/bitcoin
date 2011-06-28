#include "guiutil.h"
#include "bitcoinaddressvalidator.h"
#include "util.h"

#include <QString>
#include <QDateTime>
#include <QDoubleValidator>
#include <QFont>
#include <QLineEdit>

QString GUIUtil::DateTimeStr(qint64 nTime)
{
    QDateTime date = QDateTime::fromTime_t((qint32)nTime);
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

bool GUIUtil::parseMoney(const QString &amount, qint64 *val_out)
{
    return ParseMoney(amount.toStdString(), *val_out);
}

QString GUIUtil::formatMoney(qint64 amount, bool plussign)
{
    return QString::fromStdString(FormatMoney(amount, plussign));
}
