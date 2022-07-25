// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REVOLT_QT_REVOLTADDRESSVALIDATOR_H
#define REVOLT_QT_REVOLTADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class RevoltAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RevoltAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Revolt address widget validator, checks for a valid revolt address.
 */
class RevoltAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RevoltAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // REVOLT_QT_REVOLTADDRESSVALIDATOR_H
