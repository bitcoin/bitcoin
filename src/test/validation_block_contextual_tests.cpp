// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <script/sign.h>
#include <test/util/block_validity.h>

#include <boost/test/unit_test.hpp>

namespace {
//! Height offset used to trigger BIP34 coinbase height mismatch (current + offset)
static constexpr int BIP34_HEIGHT_OFFSET = 2;
//! Arbitrary future locktime used for non-finality checks
static constexpr uint32_t FUTURE_LOCKTIME = 1000000;
//! Number of large transactions required to exceed the 4,000,000 weight limit
static constexpr int NUM_OVERWEIGHT_TXNS = 4;
//! Size of a large witness stack (1MB) used to bloat block weight
static constexpr size_t LARGE_WITNESS_SIZE = 1000000;
//! Number of OP_CHECKSIG operations required to exceed the 80,000 sigops limit
static constexpr int EXCESSIVE_SIGOPS_COUNT = 20001;
static void CheckBlockInvalid(const BlockValidationState& state, BlockValidationResult expected_result, const std::string& expected_reason, const std::string& expected_debug_message)
{
    BOOST_CHECK_MESSAGE(state.IsInvalid(), "Expected block to be invalid but it is valid");
    BOOST_CHECK(state.GetResult() == expected_result);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), expected_reason);
    BOOST_CHECK_EQUAL(state.GetDebugMessage(), expected_debug_message);
}
}

/*
 * Tests for block validation rules that depend on the block's context in the
 * chain (ContextualCheckBlock), such as time-locks and soft-fork activation.
 */
BOOST_FIXTURE_TEST_SUITE(validation_block_contextual_tests, ValidationBlockValidityTestingSetup)

BOOST_AUTO_TEST_CASE(tbv_immature_coinbase)
{
    // Coinbase tx outputs cannot be spent until they have 100 confirmations (COINBASE_MATURITY).
    CBlock block = MakeBlock();
    block.vtx.emplace_back(MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.back(), 0, COINBASE_MATURITY, coinbaseKey, CScript() << OP_TRUE, CAmount(1 * COIN), false)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-premature-spend-of-coinbase";
    const auto debug = strprintf("tried to spend coinbase at depth 1 in transaction %s", block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_cb_amount)
{
    // The coinbase reward (subsidy + fees) cannot be exceeded.
    CBlock block = MakeBlock();
    CMutableTransaction mtx(*block.vtx[0]);
    mtx.vout[0].nValue += 1;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-cb-amount";
    const auto debug = strprintf("coinbase pays too much (actual=%d vs limit=%d)", block.vtx[0]->GetValueOut(), 50 * COIN);
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_cb_height)
{
    // BIP34: The coinbase scriptSig must start with the block height.
    CBlock block = MakeBlock();
    CMutableTransaction mtx(*block.vtx[0]);
    mtx.vin[0].scriptSig = CScript() << (WITH_LOCK(cs_main, return m_chainstate.m_chain.Height()) + BIP34_HEIGHT_OFFSET) << OP_0;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-cb-height";
    const auto debug = "block height mismatch in coinbase";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_nonfinal)
{
    // Transactions with time-locks (nLockTime) that are not yet satisfied are rejected.
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    mtx.vin[0].nSequence = 0;
    mtx.nLockTime = FUTURE_LOCKTIME;
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-nonfinal";
    const auto debug = "non-final transaction";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bip113_locktime_uses_mtp)
{
    // BIP113: nLockTime is evaluated against the median time of the previous 11 blocks,
    // not the block's header time. A transaction is non-final if its nLockTime >= MTP,
    // even when nLockTime < the current block's nTime.
    CBlock block = MakeBlock();
    const int64_t mtp = WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()->GetMedianTimePast());
    const auto outpoint = AddCoin(CScript() << OP_TRUE, 50 * COIN);
    if (!outpoint) return;
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = *outpoint;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 49 * COIN;
    // nLockTime exactly equal to MTP: non-final because IsFinalTx requires strictly less than MTP
    mtx.vin[0].nSequence = 0;
    mtx.nLockTime = static_cast<uint32_t>(mtp);
    block.vtx.push_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-nonfinal";
    const auto debug = "non-final transaction";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_blk_weight)
{
    // Blocks exceeding the maximum weight (4,000,000) are rejected.
    // We use large witnesses to exceed weight without hitting the basic size limit first.
    CBlock block = MakeBlock();
    for (int tx_idx = 0; tx_idx < NUM_OVERWEIGHT_TXNS; ++tx_idx) {
        auto [mtx, _] = CreateValidTransaction(
            /*input_transactions=*/{m_coinbase_txns[tx_idx]},
            /*inputs=*/{COutPoint(m_coinbase_txns[tx_idx]->GetHash(), 0)},
            /*input_height=*/1,
            /*input_signing_keys=*/{coinbaseKey},
            /*outputs=*/{CTxOut(49 * COIN, CScript() << OP_TRUE)},
            /*feerate=*/std::nullopt,
            /*fee_output=*/std::nullopt);
        mtx.vin[0].scriptWitness.stack.assign(1, std::vector<unsigned char>(LARGE_WITNESS_SIZE, 0));
        block.vtx.push_back(MakeTransactionRef(mtx));
    }
    m_node.chainman->GenerateCoinbaseCommitment(block, WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-blk-weight";
    const auto debug = "ContextualCheckBlock : weight limit failed";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_blk_sigops)
{
    // Blocks are limited to 80,000 signature operations to prevent DoS attacks via CPU exhaustion.
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript())},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    mtx.vout[0].scriptPubKey.clear();
    for (int sigop_idx = 0; sigop_idx < EXCESSIVE_SIGOPS_COUNT; ++sigop_idx)
        mtx.vout[0].scriptPubKey << OP_CHECKSIG;
    // Re-sign because scriptPubKey changed
    std::map<COutPoint, Coin> input_coins;
    input_coins.insert({mtx.vin[0].prevout, Coin(m_coinbase_txns[0]->vout[0], 1, false)});
    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);
    std::map<int, bilingual_str> input_errors;
    assert(SignTransaction(mtx, &keystore, input_coins, SIGHASH_ALL, input_errors));
    block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-blk-sigops";
    const auto debug = "out-of-bounds SigOpCount";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_SUITE_END()
