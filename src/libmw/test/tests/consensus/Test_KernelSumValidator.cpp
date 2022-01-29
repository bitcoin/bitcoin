// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/consensus/KernelSumValidator.h>
#include <mw/consensus/Aggregation.h>
#include <mw/crypto/Pedersen.h>
#include <mw/node/CoinsView.h>

#include <test_framework/TestMWEB.h>
#include <test_framework/models/Tx.h>
#include <test_framework/Miner.h>
#include <test_framework/TxBuilder.h>

// FUTURE: Create official test vectors for the consensus rules being tested

BOOST_FIXTURE_TEST_SUITE(TestKernelSumValidator, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(ValidateState)
{
    // Pegin transaction
    test::Tx pegin_tx = test::TxBuilder()
        .AddOutput(4'000'000)
        .AddOutput(5'000'000)
        .AddPeginKernel(9'000'000)
        .Build();

    // Standard transaction
    test::Tx standard_tx = test::TxBuilder()
        .AddInput(pegin_tx.GetOutputs()[0])
        .AddOutput(2'950'000).AddOutput(1'000'000)
        .AddPlainKernel(50'000)
        .Build();

    // Pegout Transaction
    test::Tx pegout_tx = test::TxBuilder()
        .AddInput(standard_tx.GetOutputs()[1])
        .AddOutput(200'000)
        .AddPegoutKernel(750'000, 50'000)
        .Build();

    std::vector<Commitment> utxo_commitments{
        pegin_tx.GetOutputs()[1].GetCommitment(),
        standard_tx.GetOutputs()[0].GetCommitment(),
        pegout_tx.GetOutputs()[0].GetCommitment()
    };
    std::vector<Kernel> kernels{
        pegin_tx.GetKernels().front(),
        standard_tx.GetKernels().front(),
        pegout_tx.GetKernels().front()
    };
    BlindingFactor total_offset = Blinds()
        .Add(pegin_tx.GetKernelOffset())
        .Add(standard_tx.GetKernelOffset())
        .Add(pegout_tx.GetKernelOffset())
        .Total();

    // State is valid
    KernelSumValidator::ValidateState(utxo_commitments, kernels, total_offset);

    // Move pegout kernel before pegin kernel in list to make supply negative in the past
    kernels = std::vector<Kernel>{
        pegout_tx.GetKernels().front(),
        pegin_tx.GetKernels().front(),
        standard_tx.GetKernels().front()
    };

    // Consensus error should be thrown
    BOOST_REQUIRE_THROW(KernelSumValidator::ValidateState(utxo_commitments, kernels, total_offset), ValidationException);
}

BOOST_AUTO_TEST_CASE(ValidateForBlock)
{
    // Standard transaction - 2 inputs, 2 outputs, 1 kernel
    mw::Transaction::CPtr tx1 = test::TxBuilder()
        .AddInput(5'000'000).AddInput(6'000'000)
        .AddOutput(4'000'000).AddOutput(6'500'000)
        .AddPlainKernel(500'000)
        .Build().GetTransaction();
    KernelSumValidator::ValidateForTx(*tx1); // Sanity check

    // Pegin transaction - 1 output, 1 kernel
    mw::Transaction::CPtr tx2 = test::TxBuilder()
        .AddOutput(8'000'000)
        .AddPeginKernel(8'000'000)
        .Build().GetTransaction();
    KernelSumValidator::ValidateForTx(*tx2); // Sanity check

    mw::Transaction::CPtr tx3 = test::TxBuilder()
        .AddInput(1'234'567).AddInput(4'000'000)
        .AddOutput(234'567)
        .AddPegoutKernel(4'500'000, 500'000)
        .Build().GetTransaction();
    KernelSumValidator::ValidateForTx(*tx3); // Sanity check

    BlindingFactor prev_total_offset = BlindingFactor::Random();
    mw::Transaction::CPtr pAggregated = Aggregation::Aggregate({ tx1, tx2, tx3 });
    KernelSumValidator::ValidateForTx(*pAggregated); // Sanity check

    BlindingFactor total_offset = Pedersen::AddBlindingFactors({ prev_total_offset, pAggregated->GetKernelOffset() });

    KernelSumValidator::ValidateForBlock(pAggregated->GetBody(), total_offset, prev_total_offset);
}

//
// This tests ValidateForBlock without using the TxBuilder utility, since in theory it could contain bugs.
// In the future, it would be even better to replace this with official test vectors, and avoid relying on Random entirely.
//
BOOST_AUTO_TEST_CASE(ValidateForBlockWithoutBuilder)
{
    std::vector<Input> inputs;
    std::vector<Output> outputs;
    std::vector<Kernel> kernels;

    // Add inputs
    BlindingFactor input1_bf = BlindingFactor::Random();
    SecretKey input1_key = SecretKey::Random();
    SecretKey input1_output_key = SecretKey::Random();
    inputs.push_back(test::TxInput::Create(input1_bf, input1_key, input1_output_key, 5'000'000).GetInput());

    BlindingFactor input2_bf = BlindingFactor::Random();
    SecretKey input2_key = SecretKey::Random();
    SecretKey input2_output_key = SecretKey::Random();
    inputs.push_back(test::TxInput::Create(input2_bf, input2_key, input2_output_key, 6'000'000).GetInput());

    // Add outputs
    SecretKey output1_sender_key = SecretKey::Random();
    test::TxOutput output1 = test::TxOutput::Create(
        output1_sender_key,
        StealthAddress::Random(),
        4'000'000
    );
    BlindingFactor output1_bf = output1.GetBlind();
    outputs.push_back(output1.GetOutput());

    SecretKey output2_sender_key = SecretKey::Random();
    test::TxOutput output2 = test::TxOutput::Create(
        output2_sender_key,
        StealthAddress::Random(),
        6'500'000
    );
    BlindingFactor output2_bf = output2.GetBlind();
    outputs.push_back(output2.GetOutput());

    // Kernel offset
    mw::Hash prev_total_offset = mw::Hash::FromHex("0123456789abcdef0123456789abcdef00000000000000000000000000000000");
    BlindingFactor tx_offset = BlindingFactor::Random();
    BlindingFactor total_offset = Blinds().Add(prev_total_offset).Add(tx_offset).Total();

    // Calculate kernel excess
    BlindingFactor excess = Blinds()
        .Add(output1_bf)
        .Add(output2_bf)
        .Sub(input1_bf)
        .Sub(input2_bf)
        .Sub(tx_offset)
        .Total();

    // Add kernel
    const CAmount fee = 500'000;
    kernels.push_back(Kernel::Create(excess, boost::none, fee, boost::none, std::vector<PegOutCoin>{}, boost::none));

    // Create Transaction
    auto pTransaction = mw::Transaction::Create(
        tx_offset,
        BlindingFactor::Random(),
        inputs,
        outputs,
        kernels
    );

    KernelSumValidator::ValidateForBlock(pTransaction->GetBody(), total_offset, prev_total_offset);
}

BOOST_AUTO_TEST_CASE(ValidateForTx)
{
    // Standard transaction - 2 inputs, 2 outputs, 1 kernel
    mw::Transaction::CPtr tx1 = test::TxBuilder()
        .AddInput(5'000'000).AddInput(6'000'000)
        .AddOutput(4'000'000).AddOutput(6'500'000)
        .AddPlainKernel(500'000)
        .Build().GetTransaction();
    KernelSumValidator::ValidateForTx(*tx1);

    // Pegin transaction - 1 output, 1 kernel
    mw::Transaction::CPtr tx2 = test::TxBuilder()
        .AddOutput(8'000'000)
        .AddPeginKernel(8'000'000)
        .Build().GetTransaction();
    KernelSumValidator::ValidateForTx(*tx2);

    // Pegout transaction - 2 inputs, 1 output, 1 kernel
    mw::Transaction::CPtr tx3 = test::TxBuilder()
        .AddInput(1'234'567).AddInput(4'000'000)
        .AddOutput(234'567)
        .AddPegoutKernel(4'500'000, 500'000)
        .Build().GetTransaction();
    KernelSumValidator::ValidateForTx(*tx3);

    // Aggregate all 3
    mw::Transaction::CPtr pAggregated = Aggregation::Aggregate({ tx1, tx2, tx3 });
    KernelSumValidator::ValidateForTx(*pAggregated);
}

BOOST_AUTO_TEST_SUITE_END()