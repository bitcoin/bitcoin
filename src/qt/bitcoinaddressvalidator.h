// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINADDRESSVALIDATOR_H
#define BITCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator.
   Corrects near-miss characters and refuses characters that are not part of base58.
 */
class BitcoinAddressValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitcoinAddressValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
    static bool validateAddress(const QString &address);
};

#endif // BITCOINADDRESSVALIDATOR_H
