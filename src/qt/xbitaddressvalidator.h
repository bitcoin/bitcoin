// Copyright (c) 2011-2014 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XBIT_QT_XBITADDRESSVALIDATOR_H
#define XBIT_QT_XBITADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class XBitAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit XBitAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** XBit address widget validator, checks for a valid xbit address.
 */
class XBitAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit XBitAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // XBIT_QT_XBITADDRESSVALIDATOR_H
