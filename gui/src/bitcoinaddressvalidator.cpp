#include "bitcoinaddressvalidator.h"

#include "base58.h"

BitcoinAddressValidator::BitcoinAddressValidator(QObject *parent) :
    QRegExpValidator(QRegExp(QString("^[")+QString(pszBase58)+QString("]+")), parent)
{
}
