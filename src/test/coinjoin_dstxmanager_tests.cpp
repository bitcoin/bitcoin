// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chain.h>
#include <coinjoin/coinjoin.h>
#include <coinjoin/common.h>
#include <coinjoin/context.h>
#include <script/script.h>
#include <uint256.h>

#include <algorithm>
#include <array>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coinjoin_dstxmanager_tests, TestingSetup)

static CCoinJoinBroadcastTx MakeDSTX(int vin_vout_count = 3)
{
    CCoinJoinBroadcastTx dstx;
    CMutableTransaction mtx;
    const int count = std::max(vin_vout_count, 1);
    for (int i = 0; i < count; ++i) {
        mtx.vin.emplace_back(COutPoint(uint256S("0a"), i));
        // Use denominated P2PKH outputs
        CScript spk; spk << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, i) << OP_EQUALVERIFY << OP_CHECKSIG;
        mtx.vout.emplace_back(CoinJoin::GetSmallestDenomination(), spk);
    }
    dstx.tx = MakeTransactionRef(mtx);
    dstx.m_protxHash = uint256::ONE;
    return dstx;
}

BOOST_AUTO_TEST_CASE(add_get_dstx)
{
    CCoinJoinBroadcastTx dstx = MakeDSTX();
    auto& man = *Assert(Assert(m_node.cj_ctx)->dstxman);
    // Not present initially
    BOOST_CHECK(!man.GetDSTX(dstx.tx->GetHash()));
    // Add
    man.AddDSTX(dstx);
    // Fetch back
    auto got = man.GetDSTX(dstx.tx->GetHash());
    BOOST_CHECK(static_cast<bool>(got));
    BOOST_CHECK_EQUAL(got.tx->GetHash().ToString(), dstx.tx->GetHash().ToString());
}

BOOST_AUTO_TEST_CASE(update_heights_block_connect_disconnect)
{
    CCoinJoinBroadcastTx dstx = MakeDSTX();
    auto& man = *Assert(Assert(m_node.cj_ctx)->dstxman);
    man.AddDSTX(dstx);

    // Create a fake block containing the tx
    auto block = std::make_shared<CBlock>();
    block->vtx.push_back(dstx.tx);
    CBlockIndex index; index.nHeight = 100; uint256 bh = uint256S("0b"); index.phashBlock = &bh;

    // Height should set to 100 on connect
    man.BlockConnected(block, &index);
    {
        auto got = man.GetDSTX(dstx.tx->GetHash());
        BOOST_CHECK(static_cast<bool>(got));
        BOOST_CHECK(got.GetConfirmedHeight().has_value());
        BOOST_CHECK_EQUAL(*got.GetConfirmedHeight(), 100);
    }

    // Height should clear on disconnect
    man.BlockDisconnected(block, nullptr);
    {
        auto got = man.GetDSTX(dstx.tx->GetHash());
        BOOST_CHECK(static_cast<bool>(got));
        BOOST_CHECK(!got.GetConfirmedHeight().has_value());
    }
}

BOOST_AUTO_TEST_SUITE_END()


