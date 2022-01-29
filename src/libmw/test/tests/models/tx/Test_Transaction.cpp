// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/consensus/KernelSumValidator.h>

#include <test_framework/Deserializer.h>
#include <test_framework/TestMWEB.h>
#include <test_framework/TxBuilder.h>

BOOST_FIXTURE_TEST_SUITE(TestTransaction, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(TxTransaction)
{
    const CAmount pegInAmount = 123;
    const CAmount fee = 5;

    mw::Transaction::CPtr tx = test::TxBuilder()
        .AddInput(20).AddInput(30)
        .AddOutput(45).AddOutput(pegInAmount)
        .AddPlainKernel(fee).AddPeginKernel(pegInAmount)
        .Build().GetTransaction();

    KernelSumValidator::ValidateForTx(*tx);

    //
    // Serialization
    //
    {
        std::vector<uint8_t> serialized = tx->Serialized();

        Deserializer deserializer(serialized);
        BOOST_REQUIRE(deserializer.Read<BlindingFactor>() == tx->GetKernelOffset());
        BOOST_REQUIRE(deserializer.Read<BlindingFactor>() == tx->GetStealthOffset());
        BOOST_REQUIRE(deserializer.Read<TxBody>() == tx->GetBody());

        Deserializer deserializer2(serialized);
        BOOST_REQUIRE(*tx == deserializer2.Read<mw::Transaction>());
    }

    //
    // Getters
    //
    {
        BOOST_REQUIRE(tx->GetTotalFee() == fee);
    }
}

BOOST_AUTO_TEST_SUITE_END()