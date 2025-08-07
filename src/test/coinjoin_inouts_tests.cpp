// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <coinjoin/coinjoin.h>
#include <coinjoin/common.h>
#include <chainlock/chainlock.h>
#include <chain.h>
#include <llmq/context.h>
#include <script/script.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coinjoin_inouts_tests, TestingSetup)

static CScript P2PKHScript(uint8_t tag = 0x01)
{
    // OP_DUP OP_HASH160 <20-byte-tag> OP_EQUALVERIFY OP_CHECKSIG
    std::array<unsigned char, 20> hash{};
    hash.fill(tag);
    CScript spk;
    spk << OP_DUP << OP_HASH160 << std::vector<unsigned char>(hash.begin(), hash.end()) << OP_EQUALVERIFY << OP_CHECKSIG;
    return spk;
}

BOOST_AUTO_TEST_CASE(broadcasttx_isvalidstructure_good_and_bad)
{
    // Good: equal vin/vout sizes, vin count >= min participants, <= max*entry_size, P2PKH outputs with standard denominations
    CCoinJoinBroadcastTx good;
    {
        CMutableTransaction mtx;
        // Use min pool participants (e.g. 3). Build 3 inputs and 3 denominated outputs
        const int participants = std::max(3, CoinJoin::GetMinPoolParticipants());
        for (int i = 0; i < participants; ++i) {
            CTxIn in;
            in.prevout = COutPoint(uint256S("01"), static_cast<uint32_t>(i));
            mtx.vin.push_back(in);
            // Pick the smallest denomination
            CTxOut out{CoinJoin::GetSmallestDenomination(), P2PKHScript(static_cast<uint8_t>(i))};
            mtx.vout.push_back(out);
        }
        good.tx = MakeTransactionRef(mtx);
        good.m_protxHash = uint256::ONE; // at least one of (outpoint, protxhash) must be set
    }
    BOOST_CHECK(good.IsValidStructure());

    // Bad: both identifiers null
    CCoinJoinBroadcastTx bad_ids = good;
    bad_ids.m_protxHash = uint256();
    bad_ids.masternodeOutpoint.SetNull();
    BOOST_CHECK(!bad_ids.IsValidStructure());

    // Bad: vin/vout size mismatch
    CCoinJoinBroadcastTx bad_sizes = good;
    {
        CMutableTransaction mtx(*good.tx);
        mtx.vout.pop_back();
        bad_sizes.tx = MakeTransactionRef(mtx);
    }
    BOOST_CHECK(!bad_sizes.IsValidStructure());

    // Bad: non-P2PKH output
    CCoinJoinBroadcastTx bad_script = good;
    {
        CMutableTransaction mtx(*good.tx);
        mtx.vout[0].scriptPubKey = CScript() << OP_RETURN << std::vector<unsigned char>{'x'};
        bad_script.tx = MakeTransactionRef(mtx);
    }
    BOOST_CHECK(!bad_script.IsValidStructure());

    // Bad: non-denominated amount
    CCoinJoinBroadcastTx bad_amount = good;
    {
        CMutableTransaction mtx(*good.tx);
        mtx.vout[0].nValue = 42; // not a valid denom
        bad_amount.tx = MakeTransactionRef(mtx);
    }
    BOOST_CHECK(!bad_amount.IsValidStructure());
}

BOOST_AUTO_TEST_CASE(queue_timeout_bounds)
{
    CCoinJoinQueue dsq;
    dsq.nDenom = CoinJoin::AmountToDenomination(CoinJoin::GetSmallestDenomination());
    dsq.m_protxHash = uint256::ONE;
    dsq.nTime = GetAdjustedTime();
    // current time -> not out of bounds
    BOOST_CHECK(!dsq.IsTimeOutOfBounds());

    // Too old (beyond COINJOIN_QUEUE_TIMEOUT)
    SetMockTime(GetTime() + (COINJOIN_QUEUE_TIMEOUT + 1));
    BOOST_CHECK(dsq.IsTimeOutOfBounds());

    // Too far in the future
    SetMockTime(GetTime() - 2 * (COINJOIN_QUEUE_TIMEOUT + 1)); // move back to anchor baseline
    dsq.nTime = GetAdjustedTime() + (COINJOIN_QUEUE_TIMEOUT + 1);
    BOOST_CHECK(dsq.IsTimeOutOfBounds());

    // Reset mock time
    SetMockTime(0);
}

BOOST_AUTO_TEST_CASE(broadcasttx_expiry_height_logic)
{
    // Build a valid-looking CCoinJoinBroadcastTx with confirmed height
    CCoinJoinBroadcastTx dstx;
    {
        CMutableTransaction mtx;
        const int participants = std::max(3, CoinJoin::GetMinPoolParticipants());
        for (int i = 0; i < participants; ++i) {
            mtx.vin.emplace_back(COutPoint(uint256S("02"), i));
            mtx.vout.emplace_back(CoinJoin::GetSmallestDenomination(), P2PKHScript(static_cast<uint8_t>(i)));
        }
        dstx.tx = MakeTransactionRef(mtx);
        dstx.m_protxHash = uint256::ONE;
        // mark as confirmed at height 100
        dstx.SetConfirmedHeight(100);
    }

    // Minimal CBlockIndex with required fields
    // Create a minimal block index to satisfy the interface
    CBlockIndex index;
    uint256 blk_hash = uint256S("03");
    index.nHeight = 125; // 125 - 100 == 25 > 24 → expired by height
    index.phashBlock = &blk_hash;
    BOOST_CHECK(dstx.IsExpired(&index, *Assert(m_node.llmq_ctx->clhandler)));
}

BOOST_AUTO_TEST_SUITE_END()


