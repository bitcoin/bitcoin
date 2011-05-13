#include "bitcoinaddressvalidator.h"

const QString BitcoinAddressValidator::valid_chars = "123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

BitcoinAddressValidator::BitcoinAddressValidator(QObject *parent) :
    QRegExpValidator(QRegExp("^["+valid_chars+"]+"), parent)
{
}
