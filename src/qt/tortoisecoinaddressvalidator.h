// Copyright (c) 2011-2020 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_QT_TORTOISECOINADDRESSVALIDATOR_H
#define TORTOISECOIN_QT_TORTOISECOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class TortoisecoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit TortoisecoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Tortoisecoin address widget validator, checks for a valid tortoisecoin address.
 */
class TortoisecoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit TortoisecoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // TORTOISECOIN_QT_TORTOISECOINADDRESSVALIDATOR_H
