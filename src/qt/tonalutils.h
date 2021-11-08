// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TONALUTILS_H
#define BITCOIN_QT_TONALUTILS_H

#include <QValidator>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

class TonalUtils
{
public:
    static bool Supported();

    static QValidator::State validate(QString&input, int&pos);

    static void ConvertFromHex(QString&);
    static void ConvertToHex(QString&);
};

#endif // BITCOIN_QT_TONALUTILS_H
