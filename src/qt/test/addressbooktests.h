// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_ADDRESSBOOKTESTS_H
#define BITCOIN_QT_TEST_ADDRESSBOOKTESTS_H

#include <QObject>
#include <QTest>

class AddressBookTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void addressBookTests();
};

#endif // BITCOIN_QT_TEST_ADDRESSBOOKTESTS_H
