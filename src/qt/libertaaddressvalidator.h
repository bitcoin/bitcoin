// Copyright (c) 2011-2014 The LibertaCore developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBERTA_QT_LIBERTAADDRESSVALIDATOR_H
#define LIBERTA_QT_LIBERTAADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class LibertaAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit LibertaAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Libertaaddress widget validator, checks for a valid liberta address.
 */
class LibertaAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit LibertaAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // LIBERTA_QT_LIBERTAADDRESSVALIDATOR_H
