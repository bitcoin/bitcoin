// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/node/CoinsView.h>
#include <mw/node/BlockValidator.h>

#include <test_framework/Miner.h>
#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestMineChain, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(MineChain)
{
    auto pDatabase = GetDB();

    auto pDBView = mw::CoinsViewDB::Open(GetDataDir(), nullptr, pDatabase);
    BOOST_REQUIRE(pDBView != nullptr);

    auto pCachedView = std::make_shared<mw::CoinsViewCache>(pDBView);

    test::Miner miner(GetDataDir());

    ///////////////////////
    // Mine Block 1
    ///////////////////////
    test::Tx block1_tx1 = test::Tx::CreatePegIn(1000);
    auto block1 = miner.MineBlock(150, { block1_tx1 });
    BOOST_REQUIRE(BlockValidator::ValidateBlock(block1.GetBlock(), {block1_tx1.GetPegInCoin()}, {}));
    pCachedView->ApplyBlock(block1.GetBlock());

    const auto& block1_tx1_output1 = block1_tx1.GetOutputs()[0];
    BOOST_REQUIRE(pDBView->GetUTXO(block1_tx1_output1.GetOutputID()) == nullptr);
    BOOST_REQUIRE(pCachedView->GetUTXO(block1_tx1_output1.GetOutputID()) != nullptr);

    ///////////////////////
    // Mine Block 2
    ///////////////////////
    test::Tx block2_tx1 = test::Tx::CreatePegIn(500);
    auto block2 = miner.MineBlock(151, { block2_tx1 });
    BOOST_REQUIRE(BlockValidator::ValidateBlock(block2.GetBlock(), {block2_tx1.GetPegInCoin()}, {}));
    pCachedView->ApplyBlock(block2.GetBlock());

    const auto& block2_tx1_output1 = block2_tx1.GetOutputs()[0];
    BOOST_REQUIRE(pDBView->GetUTXO(block2_tx1_output1.GetOutputID()) == nullptr);
    BOOST_REQUIRE(pCachedView->GetUTXO(block2_tx1_output1.GetOutputID()) != nullptr);

    ///////////////////////
    // Flush View
    ///////////////////////
    auto pBatch = pDatabase->CreateBatch();
    pCachedView->Flush(pBatch);
    pBatch->Commit();

    BOOST_REQUIRE(pDBView->GetUTXO(block1_tx1_output1.GetOutputID()) != nullptr);
    BOOST_REQUIRE(pCachedView->GetUTXO(block1_tx1_output1.GetOutputID()) != nullptr);
    BOOST_REQUIRE(pDBView->GetUTXO(block2_tx1_output1.GetOutputID()) != nullptr);
    BOOST_REQUIRE(pCachedView->GetUTXO(block2_tx1_output1.GetOutputID()) != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()