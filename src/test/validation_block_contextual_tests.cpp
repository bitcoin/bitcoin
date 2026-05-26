// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <node/miner.h>
#include <script/sign.h>
#include <test/block_validity_checks.h>
#include <test/util/block_validity.h>

#include <boost/test/unit_test.hpp>

using node::RegenerateCommitments;

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
    RegenerateCommitments(block, *m_node.chainman);
    const auto reason = "bad-txns-premature-spend-of-coinbase";
    const auto debug = strprintf("tried to spend coinbase at depth 1 in transaction %s", block.vtx.back()->GetHash().ToString());
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    CheckBlockInvalid(ConnectBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
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
    CheckBlockInvalid(ConnectBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_cb_height)
{
    // BIP34: The coinbase scriptSig must start with the block height.
    CBlock block = MakeBlock();
    CMutableTransaction mtx(*block.vtx[0]);
    // Push wrong height into coinbase scriptSig
    const int height_offset{2};
    mtx.vin[0].scriptSig = CScript() << (WITH_LOCK(cs_main, return m_chainstate.m_chain.Height()) + height_offset) << OP_0;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-cb-height";
    const auto debug = "block height mismatch in coinbase";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_nonfinal)
{
    // Transactions with time-locks (nLockTime) that are not yet satisfied are rejected.
    CBlock block = MakeBlock();
    const auto outpoint = AddCoin(CScript() << OP_TRUE, 50 * COIN);
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = outpoint;
    mtx.vin[0].nSequence = 0;
    mtx.vout.resize(1);
    mtx.vout[0] = CTxOut(49 * COIN, CScript() << OP_TRUE);
    // Arbitrary future locktime that exceeds MTP
    mtx.nLockTime = 1000000;
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-nonfinal";
    const auto debug = "non-final transaction";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bip113_locktime_uses_mtp)
{
    // BIP113: nLockTime is evaluated against the median time of the previous 11 blocks,
    // not the block's header time. A transaction is non-final if its nLockTime >= MTP,
    // even when nLockTime < the current block's nTime.
    CBlock block = MakeBlock();
    const int64_t mtp = WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()->GetMedianTimePast());
    const auto outpoint = AddCoin(CScript() << OP_TRUE, 50 * COIN);
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = outpoint;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 49 * COIN;
    // nLockTime exactly equal to MTP: non-final because IsFinalTx requires strictly less than MTP
    mtx.vin[0].nSequence = 0;
    mtx.nLockTime = static_cast<uint32_t>(mtp);
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-nonfinal";
    const auto debug = "non-final transaction";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_blk_weight)
{
    // Blocks exceeding the maximum weight (4,000,000) are rejected.
    // We use large witnesses to exceed weight without hitting the basic size limit first.
    // Taproot script-path spends with an OP_SUCCESS leaf allow 1 MB witness items:
    // OP_SUCCESS short-circuits script execution before the 520-byte per-item size
    // check in ExecuteWitnessScript.
    const XOnlyPubKey internal_key{coinbaseKey.GetPubKey()};
    const CScript success_leaf = CScript() << static_cast<opcodetype>(0xc8);
    TaprootBuilder taproot_builder;
    taproot_builder.Add(/*depth=*/0, success_leaf, TAPROOT_LEAF_TAPSCRIPT).Finalize(internal_key);
    Assert(taproot_builder.IsComplete());
    const WitnessV1Taproot output_key = taproot_builder.GetOutput();
    const CScript p2tr = CScript() << OP_1 << std::vector<unsigned char>(output_key.begin(), output_key.end());
    const TaprootSpendData spend_data = taproot_builder.GetSpendData();
    const std::vector<unsigned char>& control_block = *spend_data.scripts.at(
                                                                             {std::vector<unsigned char>(success_leaf.begin(), success_leaf.end()), TAPROOT_LEAF_TAPSCRIPT})
                                                           .begin();
    CBlock block = MakeBlock();
    // 4 txns with ~1MB witness each exceeds 4MW limit
    const int num_overweight_txns{4};
    for (int tx_idx = 0; tx_idx < num_overweight_txns; ++tx_idx) {
        const auto outpoint = AddCoin(p2tr, 50 * COIN);
        CMutableTransaction mtx;
        mtx.vin.resize(1);
        mtx.vin[0].prevout = outpoint;
        mtx.vout.resize(1);
        mtx.vout[0].nValue = 49 * COIN;
        // 1MB witness item, allowed by OP_SUCCESS bypassing the 520-byte limit
        const size_t large_witness_size{1000000};
        mtx.vin[0].scriptWitness.stack = {
            std::vector<unsigned char>(large_witness_size, 0),
            std::vector<unsigned char>(success_leaf.begin(), success_leaf.end()),
            control_block,
        };
        block.vtx.emplace_back(MakeTransactionRef(std::move(mtx)));
    }
    RegenerateCommitments(block, *m_node.chainman);
    const auto reason = "bad-blk-weight";
    const auto debug = "ContextualCheckBlock : weight limit failed";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_blk_sigops)
{
    // Blocks are limited to MAX_BLOCK_SIGOPS_COST (80,000) sigop cost units; each non-witness OP_CHECKSIG
    // counts as WITNESS_SCALE_FACTOR (4), so 20,001 ops × 4 = 80,004 exceeds the limit.
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
    // Each non-witness OP_CHECKSIG costs WITNESS_SCALE_FACTOR sigop units
    const int excessive_sigops_count = MAX_BLOCK_SIGOPS_COST / WITNESS_SCALE_FACTOR + 1;
    for (int sigop_idx = 0; sigop_idx < excessive_sigops_count; ++sigop_idx)
        mtx.vout[0].scriptPubKey << OP_CHECKSIG;
    // Re-sign because scriptPubKey changed
    std::map<COutPoint, Coin> input_coins;
    input_coins.insert({mtx.vin[0].prevout, Coin(m_coinbase_txns[0]->vout[0], 1, false)});
    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);
    std::map<int, bilingual_str> input_errors;
    assert(SignTransaction(mtx, &keystore, input_coins, SignOptions{.sighash_type = SIGHASH_ALL}, input_errors));
    block.vtx.emplace_back(MakeTransactionRef(std::move(mtx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-blk-sigops";
    const auto debug = "out-of-bounds SigOpCount";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_SUITE_END()
