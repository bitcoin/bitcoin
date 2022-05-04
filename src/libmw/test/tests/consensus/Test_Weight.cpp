// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/consensus/Weight.h>
#include <random.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestWeight, MWEBTestingSetup)

static std::vector<Output> CreateStandardOutputs(const size_t num_outputs)
{
    std::vector<Output> outputs;
    for (size_t i = 0; i < num_outputs; i++) {
        OutputMessage message;
        message.features = (uint8_t)GetRand(UINT8_MAX) | OutputMessage::STANDARD_FIELDS_FEATURE_BIT;

        Output standard_output(
            Commitment{},
            PublicKey{},
            PublicKey{},
            std::move(message),
            std::make_shared<RangeProof>(),
            Signature{});
        outputs.push_back(std::move(standard_output));
    }

    return outputs;
}

static Kernel CreateKernel(const bool with_stealth, const std::vector<PegOutCoin>& pegouts = {})
{
    return Kernel(
        0,
        boost::none,
        boost::none,
        pegouts,
        boost::none,
        with_stealth ? boost::make_optional(PublicKey()) : boost::none,
        std::vector<uint8_t>{},
        Commitment(),
        Signature()
    );
}

static std::vector<Kernel> CreateKernels(const size_t plain_kernels, const size_t stealth_kernels)
{
    std::vector<Kernel> kernels;
    for (size_t i = 0; i < plain_kernels; i++) {
        kernels.push_back(CreateKernel(false));
    }

    for (size_t i = 0; i < stealth_kernels; i++) {
        kernels.push_back(CreateKernel(true));
    }

    return kernels;
}

BOOST_AUTO_TEST_CASE(ExceedsMaximum)
{
    BOOST_CHECK(mw::MAX_NUM_INPUTS == 50'000);
    BOOST_CHECK(mw::BASE_KERNEL_WEIGHT == 2);
    BOOST_CHECK(mw::KERNEL_WITH_STEALTH_WEIGHT == 3);
    BOOST_CHECK(mw::BASE_OUTPUT_WEIGHT == 17);
    BOOST_CHECK(mw::STANDARD_OUTPUT_WEIGHT == 18);
    BOOST_CHECK(mw::BYTES_PER_WEIGHT == 42);
    BOOST_CHECK(mw::MAX_BLOCK_WEIGHT == 200'000);


    // 10,000 outputs + 10,000 plain kernels = 200,000 Weight
    {
        std::vector<Input> inputs(mw::MAX_NUM_INPUTS);
        std::vector<Output> outputs = CreateStandardOutputs(10'000);
        std::vector<Kernel> kernels = CreateKernels(10'000, 0);

        TxBody tx(inputs, outputs, kernels);

        BOOST_CHECK(Weight::Calculate(tx) == 200'000);
        BOOST_CHECK(!Weight::ExceedsMaximum(tx));
    }

    // 50,000 inputs max, so 50,001 should exceed maximum
    {
        std::vector<Input> inputs(mw::MAX_NUM_INPUTS + 1);
        std::vector<Output> outputs = CreateStandardOutputs(10'000);
        std::vector<Kernel> kernels = CreateKernels(10'000, 0);
        BOOST_CHECK(inputs.size() == 50'001);

        TxBody tx(inputs, outputs, kernels);

        BOOST_CHECK(Weight::Calculate(tx) == 200'000);
        BOOST_CHECK(Weight::ExceedsMaximum(tx));
    }

    // 10,000 outputs + 10,000 plain kernels and 1 stealth kernel = 200,003 Weight
    {
        std::vector<Input> inputs(mw::MAX_NUM_INPUTS);
        std::vector<Output> outputs = CreateStandardOutputs(10'000);
        std::vector<Kernel> kernels = CreateKernels(10'000, 1);
        BOOST_CHECK(inputs.size() == 50'000);

        TxBody tx(inputs, outputs, kernels);

        BOOST_CHECK(Weight::Calculate(tx) == 200'003);
        BOOST_CHECK(Weight::ExceedsMaximum(tx));
    }

    // TODO: Add tests for:
    // * OutputMessage 'extra_data'
    // * Kernel 'extra_data'
    // * Kernel pegouts of varying sizes
    // * Test all combinations of outputs, and kernels (plain and stealth) to make sure we don't exceed MAX_BLOCK_SERIALIZED_SIZE_WITH_MWEB
}

BOOST_AUTO_TEST_SUITE_END()