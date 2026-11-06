// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <test/block_validity_checks.h>
#include <test/util/block_validity.h>

#include <boost/test/unit_test.hpp>

/*
 * Tests for stateless block validation rules (CheckBlock), verifying the
 * internal structure of blocks and transactions without chain or UTXO context.
 */
BOOST_FIXTURE_TEST_SUITE(validation_block_stateless_tests, ValidationBlockValidityTestingSetup)

BOOST_AUTO_TEST_CASE(tbv_bad_merkle_root)
{
    // The merkle root in the header must match the calculated root of all transactions in the block.
    CBlock block = MakeBlock();
    block.hashMerkleRoot = uint256::ONE;
    const auto reason = "bad-txnmrklroot";
    const auto debug = "hashMerkleRoot mismatch";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
    // ConnectBlock calls CheckBlock with fCheckMerkleRoot=false, so the merkle root is not verified.
    CheckBlockValid(ConnectBlock(block));
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_duplicate_txs_CVE_2012_2459)
{
    // Merkle tree malleability check (CVE-2012-2459)
    CBlock block = MakeBlock();
    block.vtx.emplace_back(block.vtx[0]);
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-duplicate";
    const auto debug = "duplicate transaction";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
    // The duplicate is a copy of the coinbase, so CheckBlock detects it as a second coinbase before
    // reaching CheckMerkleRoot (where the general duplicate-txns check lives).
    CheckBlockInvalid(ConnectBlock(block), BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-multiple", "more than one coinbase");
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_no_transactions)
{
    // A block must contain a coinbase transaction to be valid.
    CBlock block = MakeBlock();
    block.vtx.clear();
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-blk-length";
    const auto debug = "size limits failed";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    CheckBlockInvalid(ConnectBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_cb_missing)
{
    // Every block must have a coinbase transaction as its first transaction.
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-cb-missing";
    const auto debug = "first tx is not coinbase";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    CheckBlockInvalid(ConnectBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_cb_multiple)
{
    // Blocks cannot have more than one coinbase transaction.
    CBlock block = MakeBlock();
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout.SetNull();
    // Arbitrary scriptSig value for the duplicate coinbase
    const int dummy_coinbase_height{123};
    mtx.vin[0].scriptSig = CScript() << dummy_coinbase_height;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 1;
    block.vtx.emplace_back(MakeTransactionRef(std::move(mtx)));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-cb-multiple";
    const auto debug = "more than one coinbase";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    CheckBlockInvalid(ConnectBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_CONSENSUS, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_cb_length)
{
    // The coinbase scriptSig must be between 2 and 100 bytes.
    CBlock block = MakeBlock();
    CMutableTransaction mtx(*block.vtx[0]);
    mtx.vin[0].scriptSig = CScript() << 1;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-cb-length";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_vin_empty)
{
    // Transactions must have at least one input.
    CBlock block = MakeBlock();
    CMutableTransaction mtx;
    mtx.vin.clear();
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 1;
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-vin-empty";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_vout_empty)
{
    // Transactions must have at least one output.
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    mtx.vout.clear();
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-vout-empty";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_vout_negative)
{
    // Transaction output values cannot be negative.
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    mtx.vout[0].nValue = -1;
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-vout-negative";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_vout_toolarge)
{
    // Individual transaction outputs cannot exceed the maximum possible supply (21M).
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    mtx.vout[0].nValue = MAX_MONEY + 1;
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-vout-toolarge";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_txouttotal_toolarge)
{
    // The sum of all outputs in a transaction cannot exceed the maximum possible supply (21M).
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(MAX_MONEY, CScript() << OP_TRUE), CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-txouttotal-toolarge";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_inputs_duplicate)
{
    // A transaction cannot spend the same UTXO more than once.
    CBlock block = MakeBlock();
    CMutableTransaction mtx;
    mtx.vin.resize(2);
    mtx.vin[0].prevout = mtx.vin[1].prevout = COutPoint(m_coinbase_txns[0]->GetHash(), 0);
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 1;
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-inputs-duplicate";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_txns_prevout_null)
{
    // Only the coinbase transaction can have a null prevout.
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    mtx.vin.resize(2);
    mtx.vin[1].prevout.SetNull();
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-txns-prevout-null";
    CheckTxViolation(TestValidity(block), block, reason);
    CheckTxViolation(ConnectBlock(block), block, reason);
    SolveBlockPoW(block);
    CheckTxViolation(ProcessNewBlock(block), block, reason);
}

BOOST_AUTO_TEST_CASE(tbv_bad_witness_nonce_size)
{
    // The coinbase witness reserved value must be exactly 32 bytes.
    CBlock block = MakeBlock();
    m_node.chainman->GenerateCoinbaseCommitment(block, WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()));
    CMutableTransaction mtx(*block.vtx[0]);
    // Must be exactly 32 bytes to be valid
    const size_t invalid_reserved_size{31};
    mtx.vin[0].scriptWitness.stack[0] = std::vector<unsigned char>(invalid_reserved_size, 0);
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    ResetBlock(block);
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-witness-nonce-size";
    const auto debug = "CheckWitnessMalleation : invalid witness reserved value size";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
    // CheckWitnessMalleation is called in ContextualCheckBlock, which ConnectBlock does not invoke.
    CheckBlockValid(ConnectBlock(block));
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_bad_witness_merkle_match)
{
    // The witness commitment must match the witness merkle root of all transactions in the block.
    CBlock block = MakeBlock();
    m_node.chainman->GenerateCoinbaseCommitment(block, WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()));
    int commitpos = GetWitnessCommitmentIndex(block);
    CMutableTransaction mtx(*block.vtx[0]);
    // Byte within the commitment to flip
    const size_t mangle_index{10};
    mtx.vout[commitpos].scriptPubKey[mangle_index] ^= 1;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    ResetBlock(block);
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "bad-witness-merkle-match";
    const auto debug = "CheckWitnessMalleation : witness merkle commitment mismatch";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
    // CheckWitnessMalleation is called in ContextualCheckBlock, which ConnectBlock does not invoke.
    CheckBlockValid(ConnectBlock(block));
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
}

BOOST_AUTO_TEST_CASE(tbv_unexpected_witness)
{
    // Witness data is only allowed in blocks that commit to it in their coinbase transaction.
    CBlock block = MakeBlock();
    auto [mtx, _] = CreateValidTransaction(
        /*input_transactions=*/{m_coinbase_txns[0]},
        /*inputs=*/{COutPoint(m_coinbase_txns[0]->GetHash(), 0)},
        /*input_height=*/1,
        /*input_signing_keys=*/{coinbaseKey},
        /*outputs=*/{CTxOut(1, CScript() << OP_TRUE)},
        /*feerate=*/std::nullopt,
        /*fee_output=*/std::nullopt);
    mtx.vin[0].scriptWitness.stack.assign(1, std::vector<unsigned char>(32, 0));
    block.vtx.emplace_back(MakeTransactionRef(mtx));
    CMutableTransaction coinbase(*block.vtx[0]);
    int commitpos = GetWitnessCommitmentIndex(block);
    if (commitpos != NO_WITNESS_COMMITMENT) coinbase.vout.erase(coinbase.vout.begin() + commitpos);
    block.vtx[0] = MakeTransactionRef(coinbase);
    ResetBlock(block);
    block.hashMerkleRoot = BlockMerkleRoot(block);
    const auto reason = "unexpected-witness";
    const auto debug = "CheckWitnessMalleation : unexpected witness data found";
    CheckBlockInvalid(TestValidity(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
    // ConnectBlock does not call ContextualCheckBlock (where CheckWitnessMalleation runs); script
    // verification catches the witness data on the non-segwit P2PK input independently.
    CheckScriptViolation(ConnectBlock(block), block, COutPoint(m_coinbase_txns[0]->GetHash(), 0),
                         "block-script-verify-flag-failed (Witness provided for non-witness script)");
    SolveBlockPoW(block);
    CheckBlockInvalid(ProcessNewBlock(block), BlockValidationResult::BLOCK_MUTATED, reason, debug);
}

BOOST_AUTO_TEST_SUITE_END()
