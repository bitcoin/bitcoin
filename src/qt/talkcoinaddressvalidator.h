// Copyright (c) 2011-2014 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TALKCOIN_QT_TALKCOINADDRESSVALIDATOR_H
#define TALKCOIN_QT_TALKCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class TalkcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit TalkcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Talkcoin address widget validator, checks for a valid talkcoin address.
 */
class TalkcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit TalkcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // TALKCOIN_QT_TALKCOINADDRESSVALIDATOR_H
