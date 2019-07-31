// Copyright (c) 2011-2014 The Bitcointalkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINTALKCOIN_QT_BITCOINTALKCOINADDRESSVALIDATOR_H
#define BITCOINTALKCOIN_QT_BITCOINTALKCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class BitcointalkcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitcointalkcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Bitcointalkcoin address widget validator, checks for a valid bitcointalkcoin address.
 */
class BitcointalkcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitcointalkcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // BITCOINTALKCOIN_QT_BITCOINTALKCOINADDRESSVALIDATOR_H
