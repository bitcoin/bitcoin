// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/models/tx/UTXO.h>
#include <mw/models/wallet/StealthAddress.h>

#include <test_framework/Deserializer.h>
#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestUTXO, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(TxUTXO)
{
    CAmount amount = 12345;
    BlindingFactor blind;
    Output output = Output::Create(
        &blind,
        SecretKey::Random(),
        StealthAddress::Random(),
        amount
    );
    Commitment commit = Commitment::Switch(blind, amount);

    int32_t blockHeight = 20;
    mmr::LeafIndex leafIndex = mmr::LeafIndex::At(5);
    UTXO utxo{
        blockHeight,
        mmr::LeafIndex(leafIndex),
        Output(output)
    };

    //
    // Serialization
    //
    {
        std::vector<uint8_t> serialized = utxo.Serialized();

        Deserializer deserializer(serialized);
        BOOST_REQUIRE(deserializer.Read<int32_t>() == blockHeight);
        BOOST_REQUIRE(mmr::LeafIndex::At(deserializer.Read<uint64_t>()) == leafIndex);
        BOOST_REQUIRE(deserializer.Read<Output>() == output);
    }

    //
    // Getters
    //
    {
        BOOST_REQUIRE(utxo.GetBlockHeight() == blockHeight);
        BOOST_REQUIRE(utxo.GetLeafIndex() == leafIndex);
        BOOST_REQUIRE(utxo.GetOutput() == output);
        BOOST_REQUIRE(utxo.GetCommitment() == commit);
        BOOST_REQUIRE(utxo.GetRangeProof() == output.GetRangeProof());
        BOOST_REQUIRE(utxo.BuildProofData() == output.BuildProofData());
    }
}

BOOST_AUTO_TEST_SUITE_END()