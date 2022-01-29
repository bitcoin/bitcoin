// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/node/BlockValidator.h>

#include <test_framework/Miner.h>
#include <test_framework/TestMWEB.h>
#include <test_framework/TxBuilder.h>

BOOST_FIXTURE_TEST_SUITE(TestBlockValidator, MWEBTestingSetup)

// MW: TODO - Write tests for invalid blocks:
// - Pegin mismatch
// - Pegout mismatch
// - Num kernels mismatch
// - Kernel root mismatch
// - Invalid stealth excess sum
// * Block weight
// * Unsorted inputs
// - Unsorted outputs
// - Unsorted kernels
// * Duplicate spent IDs
// * Duplicate output IDs
// * Duplicate kernel IDs
// * Invalid input signature
// * Invalid output signature
// * Invalid kernel signature
// * Invalid rangeproof

BOOST_AUTO_TEST_CASE(BlockValidator_Test_ValidBlock)
{
    test::Miner miner(GetDataDir());

    // Block 1 - 1 pegin & 1 pegout
    test::Tx pegin_tx = test::Tx::CreatePegIn(5'000'000);
    test::Tx pegout_tx = test::Tx::CreatePegOut(pegin_tx.GetOutputs().front());
    mw::Block::CPtr pBlock = miner.MineBlock(1, { pegin_tx, pegout_tx }).GetBlock();

    bool is_valid = BlockValidator::ValidateBlock(
        pBlock,
        std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
        std::vector<PegOutCoin>{pegout_tx.GetPegOutCoin()}
    );
    BOOST_CHECK(is_valid);

    // Block 2 - Empty
    mw::Block::CPtr pEmptyBlock = miner.MineBlock(2).GetBlock();

    is_valid = BlockValidator::ValidateBlock(
        pEmptyBlock,
        std::vector<PegInCoin>{},
        std::vector<PegOutCoin>{}
    );
    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(BlockValidator_Test_PeginMismatch)
{
    test::Miner miner(GetDataDir());

    {
        test::Tx pegin_tx = test::Tx::CreatePegIn(5'000'000);
        mw::Block::CPtr pBlock = miner.MineBlock(1, {pegin_tx}).GetBlock();

        bool is_valid = BlockValidator::ValidateBlock(
            pBlock,
            std::vector<PegInCoin>{},
            std::vector<PegOutCoin>{}
        );
        BOOST_CHECK(!is_valid);
    }

    {
        test::Tx pegin_tx = test::Tx::CreatePegIn(5'000'000);
        mw::Block::CPtr pBlock = miner.MineBlock(2, {}).GetBlock();

        bool is_valid = BlockValidator::ValidateBlock(
            pBlock,
            std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
            std::vector<PegOutCoin>{}
        );
        BOOST_CHECK(!is_valid);
    }
}

BOOST_AUTO_TEST_CASE(BlockValidator_Test_PegoutMismatch)
{
    test::Miner miner(GetDataDir());

    {
        test::Tx pegin_tx = test::Tx::CreatePegIn(5'000'000);
        test::Tx pegout_tx = test::Tx::CreatePegOut(pegin_tx.GetOutputs().front());
        mw::Block::CPtr pBlock = miner.MineBlock(1, {pegin_tx, pegout_tx}).GetBlock();

        bool is_valid = BlockValidator::ValidateBlock(
            pBlock,
            std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
            std::vector<PegOutCoin>{}
        );
        BOOST_CHECK(!is_valid);
    }
    
    {
        test::Tx pegin_tx = test::Tx::CreatePegIn(5'000'000);
        test::Tx pegout_tx = test::Tx::CreatePegOut(pegin_tx.GetOutputs().front());
        mw::Block::CPtr pBlock = miner.MineBlock(1, {pegin_tx}).GetBlock();

        bool is_valid = BlockValidator::ValidateBlock(
            pBlock,
            std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
            std::vector<PegOutCoin>{pegout_tx.GetPegOutCoin()}
        );
        BOOST_CHECK(!is_valid);
    }
}

BOOST_AUTO_TEST_CASE(BlockValidator_Test_KernelMismatch)
{
    test::Miner miner(GetDataDir());

    test::Tx pegin_tx = test::Tx::CreatePegIn(5'000'000);
    mw::Block::CPtr pBlock = miner.MineBlock(1, { pegin_tx }).GetBlock();

    bool is_valid = BlockValidator::ValidateBlock(
        pBlock,
        std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
        std::vector<PegOutCoin>{}
    );
    BOOST_CHECK(is_valid);
    
    // Kernel root mismatch
    mw::Block::CPtr pBlockKernelRootMismatch = std::make_shared<mw::Block>(
        mw::MutHeader(pBlock->GetHeader())
            .SetKernelRoot(SecretKey::Random().GetBigInt())
            .Build(),
        pBlock->GetTxBody()
    );

    is_valid = BlockValidator::ValidateBlock(
        pBlockKernelRootMismatch,
        std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
        std::vector<PegOutCoin>{}
    );
    BOOST_CHECK(!is_valid);

    
    // Num kernels mismatch
    mw::Block::CPtr pBlockNumKernelsMismatch = std::make_shared<mw::Block>(
        mw::MutHeader(pBlock->GetHeader())
            .SetNumKernels(10)
            .Build(),
        pBlock->GetTxBody()
    );

    is_valid = BlockValidator::ValidateBlock(
        pBlockNumKernelsMismatch,
        std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
        std::vector<PegOutCoin>{}
    );
    BOOST_CHECK(!is_valid);
}

BOOST_AUTO_TEST_CASE(BlockValidator_Test_InvalidStealthExcess)
{
    test::Miner miner(GetDataDir());

    test::Tx pegin_tx = test::Tx::CreatePegIn(5'000'000);
    mw::Block::CPtr pBlock = miner.MineBlock(1, {pegin_tx}).GetBlock();

    bool is_valid = BlockValidator::ValidateBlock(
        pBlock,
        std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
        std::vector<PegOutCoin>{});
    BOOST_CHECK(is_valid);

    // Stealth excess invalid
    mw::Block::CPtr pBlockKernelRootMismatch = std::make_shared<mw::Block>(
        mw::MutHeader(pBlock->GetHeader())
            .SetStealthOffset(SecretKey::Random().GetBigInt())
            .Build(),
        pBlock->GetTxBody()
    );

    is_valid = BlockValidator::ValidateBlock(
        pBlockKernelRootMismatch,
        std::vector<PegInCoin>{pegin_tx.GetPegInCoin()},
        std::vector<PegOutCoin>{}
    );
    BOOST_CHECK(!is_valid);
}

BOOST_AUTO_TEST_CASE(BlockValidator_Test_OutputSorting)
{
    test::Miner miner(GetDataDir());

    test::Tx tx = test::TxBuilder()
        .AddPeginKernel(5'000'000)
        .AddOutput(3'000'000, SecretKey::Random(), StealthAddress::Random())
        .AddOutput(2'000'000, SecretKey::Random(), StealthAddress::Random())
        .Build();
    mw::Block::CPtr pBlock = miner.MineBlock(1, {tx}).GetBlock();

    bool is_valid = BlockValidator::ValidateBlock(
        pBlock,
        tx.GetPegIns(),
        tx.GetPegOuts()
    );
    BOOST_CHECK(is_valid);

    // Swap outputs so they're no longer in order
    std::vector<Output> outputs = pBlock->GetOutputs();
    Output tmp = outputs[0];
    outputs[0] = outputs[1];
    outputs[1] = tmp;

    BOOST_REQUIRE(outputs[0].GetOutputID() > outputs[1].GetOutputID());
    mw::Block::CPtr pUnsortedBlock = mw::MutBlock(pBlock)
        .SetOutputs(std::move(outputs))
        .Build();

    is_valid = BlockValidator::ValidateBlock(
        pUnsortedBlock,
        tx.GetPegIns(),
        tx.GetPegOuts()
    );
    BOOST_CHECK(!is_valid);
}

BOOST_AUTO_TEST_CASE(BlockValidator_Test_KernelSorting)
{
    test::Miner miner(GetDataDir());

    test::Tx tx = test::TxBuilder()
        .AddPeginKernel(5'000'000)
        .AddPeginKernel(10'000'000)
        .AddOutput(15'000'000, SecretKey::Random(), StealthAddress::Random())
        .Build();
    mw::Block::CPtr pBlock = miner.MineBlock(1, {tx}).GetBlock();

    bool is_valid = BlockValidator::ValidateBlock(
        pBlock,
        tx.GetPegIns(),
        tx.GetPegOuts()
    );
    BOOST_CHECK(is_valid);

    // Swap kernels so they're no longer in order
    std::vector<Kernel> kernels = pBlock->GetKernels();
    Kernel tmp = kernels[0];
    kernels[0] = kernels[1];
    kernels[1] = tmp;

    BOOST_REQUIRE(kernels[0].GetSupplyChange() < kernels[1].GetSupplyChange());
    mw::Block::CPtr pUnsortedBlock = mw::MutBlock(pBlock)
        .SetKernels(std::move(kernels))
        .Build();

    is_valid = BlockValidator::ValidateBlock(
        pUnsortedBlock,
        tx.GetPegIns(),
        tx.GetPegOuts()
    );
    BOOST_CHECK(!is_valid);
}

BOOST_AUTO_TEST_SUITE_END()