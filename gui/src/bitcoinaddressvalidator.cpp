#include "bitcoinaddressvalidator.h"

#include "base58.h"

#include <QDebug>

/* Base58 characters are:
     "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

  This is:
  - All numbers except for '0'
  - All uppercase letters except for 'I' and 'O'
  - All lowercase letters except for 'l'

  User friendly Base58 input can map
  - 'l' and 'I' to '1'
  - '0' and 'O' to 'o'
*/

BitcoinAddressValidator::BitcoinAddressValidator(QObject *parent) :
    QRegExpValidator(QRegExp(QString("^[")+QString(pszBase58)+QString("]+")), parent)
{
}

QValidator::State BitcoinAddressValidator::validate(QString &input, int &pos) const
{
    for(int idx=0; idx<input.size(); ++idx)
    {
        switch(input.at(idx).unicode())
        {
        case 'l':
        case 'I':
            input[idx] = QChar('1');
            break;
        case '0':
        case 'O':
            input[idx] = QChar('o');
            break;
        default:
            break;
        }

    }
    return QRegExpValidator::validate(input, pos);
}
