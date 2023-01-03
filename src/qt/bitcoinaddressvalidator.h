// Copyright (c) 2011-2014 The Buttcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef Buttcoin_QT_ButtcoinADDRESSVALIDATOR_H
#define Buttcoin_QT_ButtcoinADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class ButtcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit ButtcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Buttcoin address widget validator, checks for a valid Buttcoin address.
 */
class ButtcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit ButtcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // Buttcoin_QT_ButtcoinADDRESSVALIDATOR_H
