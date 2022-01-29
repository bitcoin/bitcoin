// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/consensus/Aggregation.h>

#include <test_framework/TestMWEB.h>
#include <test_framework/TxBuilder.h>

BOOST_FIXTURE_TEST_SUITE(TestAggregation, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(Aggregate)
{
    mw::Transaction::CPtr tx1 = test::TxBuilder()
        .AddInput(10).AddInput(20)
        .AddOutput(25).AddPlainKernel(5, true)
        .Build().GetTransaction();
    tx1->Validate();

    mw::Transaction::CPtr tx2 = test::TxBuilder()
        .AddInput(20)
        .AddOutput(15).AddPlainKernel(5, true)
        .Build().GetTransaction();
    tx2->Validate();

    mw::Transaction::CPtr pAggregated = Aggregation::Aggregate({ tx1, tx2 });
    pAggregated->Validate();

    std::vector<Input> inputs = tx1->GetInputs();
    inputs.insert(inputs.end(), tx2->GetInputs().begin(), tx2->GetInputs().end());
    std::sort(inputs.begin(), inputs.end(), InputSort);
    BOOST_REQUIRE(pAggregated->GetInputs().size() == 3);
    BOOST_REQUIRE(pAggregated->GetInputs() == inputs);

    std::vector<Output> outputs = tx1->GetOutputs();
    outputs.insert(outputs.end(), tx2->GetOutputs().begin(), tx2->GetOutputs().end());
    std::sort(outputs.begin(), outputs.end(), OutputSort);
    BOOST_REQUIRE(pAggregated->GetOutputs().size() == 2);
    BOOST_REQUIRE(pAggregated->GetOutputs() == outputs);

    std::vector<Kernel> kernels = tx1->GetKernels();
    kernels.insert(kernels.end(), tx2->GetKernels().begin(), tx2->GetKernels().end());
    std::sort(kernels.begin(), kernels.end(), KernelSort);
    BOOST_REQUIRE(pAggregated->GetKernels().size() == 2);
    BOOST_REQUIRE(pAggregated->GetKernels() == kernels);

    BlindingFactor kernel_offset = Blinds()
        .Add(tx1->GetKernelOffset())
        .Add(tx2->GetKernelOffset())
        .Total();
    BOOST_REQUIRE(pAggregated->GetKernelOffset() == kernel_offset);

    BlindingFactor stealth_offset = Blinds()
        .Add(tx1->GetStealthOffset())
        .Add(tx2->GetStealthOffset())
        .Total();
    BOOST_REQUIRE(pAggregated->GetStealthOffset() == stealth_offset);
}

BOOST_AUTO_TEST_SUITE_END()