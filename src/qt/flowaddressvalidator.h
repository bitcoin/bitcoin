// Copyright (c) 2011-2014 The Flow Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FLOW_QT_FLOWADDRESSVALIDATOR_H
#define FLOW_QT_FLOWADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class FlowAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FlowAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Flow address widget validator, checks for a valid flow address.
 */
class FlowAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FlowAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // FLOW_QT_FLOWADDRESSVALIDATOR_H
