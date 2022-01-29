// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/models/tx/PegOutCoin.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestPegOutCoin, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(TxPegOutCoin)
{
    CAmount amount = 123;
    std::vector<uint8_t> vpubkey = PublicKey::Random().vec();
    CScript scriptPubKey(vpubkey.begin(), vpubkey.end());
    PegOutCoin pegOutCoin(amount, scriptPubKey);

    //
    // Serialization
    //
    {
        PegOutCoin pegOutCoin2;
        CDataStream(pegOutCoin.Serialized(), SER_DISK, PROTOCOL_VERSION) >> pegOutCoin2;
        BOOST_REQUIRE(pegOutCoin == pegOutCoin2);
    }

    //
    // Getters
    //
    {
        BOOST_REQUIRE(pegOutCoin.GetAmount() == amount);
        BOOST_REQUIRE(pegOutCoin.GetScriptPubKey() == scriptPubKey);
    }
}

BOOST_AUTO_TEST_SUITE_END()