// Copyright (c) 2011-2014 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_QT_SYSCOINADDRESSVALIDATOR_H
#define SYSCOIN_QT_SYSCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class SyscoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit SyscoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Syscoin address widget validator, checks for a valid syscoin address.
 */
class SyscoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit SyscoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // SYSCOIN_QT_SYSCOINADDRESSVALIDATOR_H
