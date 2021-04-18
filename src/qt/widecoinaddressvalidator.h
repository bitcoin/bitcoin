// Copyright (c) 2011-2014 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_QT_WIDECOINADDRESSVALIDATOR_H
#define WIDECOIN_QT_WIDECOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class WidecoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit WidecoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Widecoin address widget validator, checks for a valid widecoin address.
 */
class WidecoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit WidecoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // WIDECOIN_QT_WIDECOINADDRESSVALIDATOR_H
