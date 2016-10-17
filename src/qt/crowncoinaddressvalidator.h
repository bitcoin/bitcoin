// Copyright (c) 2011-2014 The Crowncoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWNCOINADDRESSVALIDATOR_H
#define CROWNCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class CrowncoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit CrowncoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Crowncoin address widget validator, checks for a valid Crowncoin address.
 */
class CrowncoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit CrowncoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // CROWNCOINADDRESSVALIDATOR_H
