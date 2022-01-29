// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/consensus/StealthSumValidator.h>
#include <mw/crypto/Blinds.h>

#include <test_framework/TestMWEB.h>
#include <test_framework/models/Tx.h>

// FUTURE: Create official test vectors for the consensus rules being tested

BOOST_FIXTURE_TEST_SUITE(TestStealthSumValidator, MWEBTestingSetup)

//
// This tests the StealthSumValidator with and without an offset.
//
BOOST_AUTO_TEST_CASE(ValidateStealthSum)
{
    ////////////////////////////////////////
    // Build inputs
    ////////////////////////////////////////
    std::vector<Input> inputs;

    SecretKey input1_key = SecretKey::Random();
    SecretKey input1_output_key = SecretKey::Random();
    Input input1(
        1,                                  // features
        SecretKey::Random().GetBigInt(),    // output_id
        Commitment::Random(),               // commitment
        PublicKey::From(input1_key),        // input pubkey
        PublicKey::From(input1_output_key), // output pubkey
        Signature()                         // input signature
    );
    inputs.push_back(std::move(input1));

    SecretKey input2_key = SecretKey::Random();
    SecretKey input2_output_key = SecretKey::Random();
    Input input2(
        1,                                  // features
        SecretKey::Random().GetBigInt(),    // output_id
        Commitment::Random(),               // commitment
        PublicKey::From(input2_key),        // input pubkey
        PublicKey::From(input2_output_key), // output pubkey
        Signature()                         // input signature
    );
    inputs.push_back(std::move(input2));

    ////////////////////////////////////////
    // Build outputs
    ////////////////////////////////////////
    std::vector<Output> outputs;

    SecretKey output1_sender_key = SecretKey::Random();
    Output output1 = Output::Create(
        nullptr,                            // (out) blinding_factor
        output1_sender_key,                 // sender private key
        StealthAddress::Random(),           // receiver_addr
        1'000'000                           // amount
    );
    outputs.push_back(std::move(output1));
    
    SecretKey output2_sender_key = SecretKey::Random();
    Output output2 = Output::Create(
        nullptr,                            // (out) blinding_factor
        output2_sender_key,                 // sender private key
        StealthAddress::Random(),           // receiver_addr
        2'000'000                           // amount
    );
    outputs.push_back(std::move(output2));

    ////////////////////////////////////////
    // Calculate stealth excess
    ////////////////////////////////////////
    SecretKey stealth_offset = SecretKey::Random();
    SecretKey stealth_excess_no_offset = Blinds()
        .Add(output1_sender_key)
        .Add(output2_sender_key)
        .Add(input1_key)
        .Add(input2_key)
        .Sub(input1_output_key)
        .Sub(input2_output_key)
        .Total().data();
    SecretKey stealth_excess_with_offset = Blinds::From(stealth_excess_no_offset)
        .Sub(stealth_offset)
        .Total().data();

    ////////////////////////////////////////
    // Build kernels
    ////////////////////////////////////////
    Kernel kernel1_no_offset = Kernel::Create(
        BlindingFactor::Random(),           // excess
        stealth_excess_no_offset,           // stealth excess
        boost::none,                        // fee
        boost::none,                        // pegin_amount
        std::vector<PegOutCoin>{},          // pegout
        boost::none                         // lock_height
    );

    Kernel kernel1_with_offset = Kernel::Create(
        BlindingFactor::Random(),           // excess
        stealth_excess_with_offset,         // stealth excess
        boost::none,                        // fee
        boost::none,                        // pegin_amount
        std::vector<PegOutCoin>{},          // pegout
        boost::none                         // lock_height
    );

    Kernel kernel2_no_stealth_excess = Kernel::Create(
        BlindingFactor::Random(),           // excess
        boost::none,                        // stealth excess
        boost::none,                        // fee
        boost::none,                        // pegin_amount
        std::vector<PegOutCoin>{},          // pegout
        boost::none                         // lock_height
    );

    // Validate stealth sums with a zero stealth offset
    StealthSumValidator::Validate(SecretKey{}, TxBody{inputs, outputs, std::vector<Kernel>{kernel1_no_offset, kernel2_no_stealth_excess}});

    // Validate stealth sums with a non-zero stealth offset
    StealthSumValidator::Validate(stealth_offset, TxBody{inputs, outputs, std::vector<Kernel>{kernel1_with_offset, kernel2_no_stealth_excess}});
}

BOOST_AUTO_TEST_SUITE_END()