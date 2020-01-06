// Copyright (c) 2016-2018 The NdovuCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_RPCNESTEDTESTS_H
#define BITCOIN_QT_TEST_RPCNESTEDTESTS_H

#include <QObject>
#include <QTest>

class RPCNestedTests : public QObject
{
    Q_OBJECT

    private Q_SLOTS:
    void rpcNestedTests();
};

#endif // BITCOIN_QT_TEST_RPCNESTEDTESTS_H
