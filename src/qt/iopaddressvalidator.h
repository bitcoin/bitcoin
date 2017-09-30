// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IOP_QT_IOPADDRESSVALIDATOR_H
#define IOP_QT_IOPADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class IoPAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit IoPAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** IoP address widget validator, checks for a valid iop address.
 */
class IoPAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit IoPAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // IOP_QT_IOPADDRESSVALIDATOR_H
