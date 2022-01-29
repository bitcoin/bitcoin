// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test_framework/TestMWEB.h>
#include <test_framework/TxBuilder.h>

BOOST_FIXTURE_TEST_SUITE(TestTxBody, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(Test_TxBody)
{
    const uint64_t pegInAmount = 123;
    const uint64_t fee = 5;

    mw::Transaction::CPtr tx = test::TxBuilder()
        .AddInput(20).AddInput(30)
        .AddOutput(45).AddOutput(pegInAmount)
        .AddPlainKernel(fee).AddPeginKernel(pegInAmount)
        .Build().GetTransaction();

    const TxBody& txBody = tx->GetBody();
    txBody.Validate();

    //
    // Serialization
    //
    std::vector<uint8_t> serialized = txBody.Serialized();
    TxBody txBody2;
    VectorReader(SER_NETWORK, PROTOCOL_VERSION, serialized, 0) >> txBody2;
    BOOST_REQUIRE(txBody == txBody2);

    //
    // Getters
    //
    BOOST_REQUIRE(txBody.GetTotalFee() == fee);
}

BOOST_AUTO_TEST_SUITE_END()