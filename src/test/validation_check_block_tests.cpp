// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <kernel/chainparams.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <uint256.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE(validation_check_block_tests)

BOOST_AUTO_TEST_CASE(block_valid)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    const CBlock block{chainparams->GenesisBlock()};

    BlockValidationState state;

    BOOST_CHECK(CheckBlock(block, state, consensusParams));
    BOOST_CHECK(state.IsValid());
    BOOST_CHECK(block.fChecked);
}

BOOST_AUTO_TEST_CASE(block_valid_multiple_transactions)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Append a transaction with a non-null prevout, so that it is not a
    // coinbase and the loop over the non-coinbase transactions completes an
    // iteration instead of returning on the first one.
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vin[0].prevout.n = 0;
    block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    BOOST_REQUIRE(!block.vtx[1]->IsCoinBase());
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;

    BOOST_CHECK(CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(block_valid_signet)
{
    const auto chainparams{CChainParams::SigNet()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    const CBlock block{chainparams->GenesisBlock()};

    BlockValidationState state;

    BOOST_CHECK(CheckBlock(block, state, consensusParams));
    BOOST_CHECK(state.IsValid());
    BOOST_CHECK(block.fChecked);
}

BOOST_AUTO_TEST_CASE(block_cached)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    BlockValidationState state;

    BOOST_REQUIRE(CheckBlock(block, state, consensusParams));
    BOOST_REQUIRE(block.fChecked);

    // Invalidate the proof of work. The block is still accepted and valid, which
    // is only possible if the cached result is returned without re-running the checks.
    block.nNonce = 0;
    BOOST_CHECK(CheckBlock(block, state, consensusParams));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(block_not_cached_on_partial_check)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    const CBlock block{chainparams->GenesisBlock()};

    BlockValidationState state;

    // Skipping either check must not mark the block as fully checked.
    BOOST_CHECK(CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(!block.fChecked);
    BOOST_CHECK(CheckBlock(block, state, consensusParams, /*fCheckPOW=*/true, /*fCheckMerkleRoot=*/false));
    BOOST_CHECK(!block.fChecked);
}

BOOST_AUTO_TEST_CASE(block_bad_header)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};
    block.nNonce = 0;

    BlockValidationState state;

    BOOST_CHECK(!CheckBlock(block, state, consensusParams));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "high-hash");
}

BOOST_AUTO_TEST_CASE(block_bad_signet_blksig)
{
    const auto chainparams{CChainParams::SigNet()};
    Consensus::Params consensusParams{chainparams->GetConsensus()};

    // The signet solution of the genesis block is never checked, so the params
    // are pointed at a different block to get it checked like any other.
    consensusParams.hashGenesisBlock = uint256{};

    const CBlock block{chainparams->GenesisBlock()};

    BlockValidationState state;

    // The coinbase carries no witness commitment, so no signet solution can be
    // extracted from it.
    BOOST_CHECK(!CheckBlock(block, state, consensusParams));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-signet-blksig");
}

BOOST_AUTO_TEST_CASE(block_bad_txnmrklroot)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};
    block.hashMerkleRoot = uint256{};

    BlockValidationState state;

    // The header commits to the merkle root, so the proof of work no longer
    // matches once it is changed.
    BOOST_CHECK(!CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-txnmrklroot");

    // The same block passes when the merkle root is not checked.
    BlockValidationState state_unchecked;
    BOOST_CHECK(CheckBlock(block, state_unchecked, consensusParams, /*fCheckPOW=*/false, /*fCheckMerkleRoot=*/false));
    BOOST_CHECK(state_unchecked.IsValid());
}

BOOST_AUTO_TEST_CASE(block_bad_blk_length_empty)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Drop all transactions. The merkle root of an empty block is the null
    // hash, so it is updated to match and the size check is reached.
    block.vtx.clear();
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;

    BOOST_CHECK(!CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-blk-length");
}

BOOST_AUTO_TEST_CASE(block_bad_blk_length_oversized)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Pad the coinbase with the maximum base size worth of data, so that the
    // block exceeds the weight limit by at least the size of its header.
    const std::vector<unsigned char> padding(MAX_BLOCK_WEIGHT / WITNESS_SCALE_FACTOR, OP_RETURN);
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vout[0].scriptPubKey = CScript(padding.begin(), padding.end());
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;

    BOOST_CHECK(!CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-blk-length");
}

BOOST_AUTO_TEST_CASE(block_bad_cb_missing)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Give the first transaction a non-null prevout so that it is no longer a
    // coinbase, and update the merkle root to match.
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vin[0].prevout.n = 0;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    BOOST_REQUIRE(!block.vtx[0]->IsCoinBase());
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;

    BOOST_CHECK(!CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-cb-missing");
}

BOOST_AUTO_TEST_CASE(block_bad_cb_multiple)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Append a second coinbase. It must not be identical to the first one, or
    // the merkle root check would reject the block as "bad-txns-duplicate"
    // before the coinbase count is looked at.
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.nLockTime = 1;
    block.vtx.push_back(MakeTransactionRef(std::move(mtx)));
    BOOST_REQUIRE(block.vtx[1]->IsCoinBase());
    BOOST_REQUIRE(block.vtx[0]->GetHash() != block.vtx[1]->GetHash());
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;

    BOOST_CHECK(!CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-cb-multiple");
}

BOOST_AUTO_TEST_CASE(block_bad_txns_vout_negative)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Give the coinbase a negative output value. The transaction stays a
    // coinbase, so the block is only rejected once its transactions are
    // checked, and the reason is the one reported by CheckTransaction().
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vout[0].nValue = -1;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    BOOST_REQUIRE(block.vtx[0]->IsCoinBase());
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;

    BOOST_CHECK(!CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-txns-vout-negative");
}

BOOST_AUTO_TEST_CASE(block_bad_blk_sigops)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Fill the coinbase output script with one more OP_CHECKSIG than the
    // block is allowed to have.
    const std::vector<unsigned char> checksigs(MAX_BLOCK_SIGOPS_COST / WITNESS_SCALE_FACTOR + 1, OP_CHECKSIG);
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vout[0].scriptPubKey = CScript(checksigs.begin(), checksigs.end());
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;

    BOOST_CHECK(!CheckBlock(block, state, consensusParams, /*fCheckPOW=*/false));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-blk-sigops");
}

BOOST_AUTO_TEST_CASE(contextual_block_valid)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    const CBlock block{chainparams->GenesisBlock()};

    BlockValidationState state;

    // Without a previous block the height is zero, so none of the deployments
    // this depends on are active yet.
    BOOST_CHECK(ContextualCheckBlock(block, state, consensusParams, /*pindexPrev=*/nullptr));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(contextual_block_bad_txns_nonfinal)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // A locktime that has not passed yet is ignored as long as every input is
    // final, so the sequence number has to be lowered as well.
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.nLockTime = block.nTime;
    mtx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL - 1;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlock(block, state, consensusParams, /*pindexPrev=*/nullptr));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-txns-nonfinal");
}

BOOST_AUTO_TEST_CASE(contextual_block_cb_height)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    CBlockIndex prev;
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_HEIGHTINCB);
    const int height{prev.nHeight + 1};

    BlockValidationState state;

    // The genesis coinbase does not start with the serialized block height.
    BOOST_CHECK(!ContextualCheckBlock(block, state, consensusParams, &prev));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-cb-height");

    // A scriptSig that is too short to hold the height is rejected on its
    // length alone, before a comparison that would read past the end of it.
    CMutableTransaction truncated{*block.vtx[0]};
    truncated.vin[0].scriptSig = CScript();
    block.vtx[0] = MakeTransactionRef(std::move(truncated));

    BlockValidationState state_truncated;
    BOOST_CHECK(!ContextualCheckBlock(block, state_truncated, consensusParams, &prev));
    BOOST_CHECK(state_truncated.IsInvalid());
    BOOST_CHECK(state_truncated.GetRejectReason() == "bad-cb-height");

    // Prefixing the coinbase with the height of this block makes it acceptable.
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vin[0].scriptSig = CScript() << height;
    block.vtx[0] = MakeTransactionRef(std::move(mtx));

    BlockValidationState state_with_height;
    BOOST_CHECK(ContextualCheckBlock(block, state_with_height, consensusParams, &prev));
    BOOST_CHECK(state_with_height.IsValid());
}

BOOST_AUTO_TEST_CASE(contextual_block_unexpected_witness)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Segwit is not active at height zero, so the block is not allowed to
    // carry any witness data at all.
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vin[0].scriptWitness.stack.emplace_back(32, 0);
    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    BOOST_REQUIRE(block.vtx[0]->HasWitness());

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlock(block, state, consensusParams, /*pindexPrev=*/nullptr));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "unexpected-witness");
}

BOOST_AUTO_TEST_CASE(contextual_block_bad_witness_nonce_size)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    CBlockIndex prev;
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_SEGWIT);
    prev.nTime = block.nTime;
    const int height{prev.nHeight + 1};

    // Give the coinbase a witness commitment, so that the witness reserved
    // value is looked at. The output script is OP_RETURN followed by a
    // 36 byte push: the four byte header and a 32 byte commitment hash. The
    // height prefix is needed because BIP34 is active by this height as well.
    std::vector<unsigned char> commitment{0xaa, 0x21, 0xa9, 0xed};
    commitment.resize(36);
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vin[0].scriptSig = CScript() << height;
    mtx.vout.emplace_back(0, CScript() << OP_RETURN << commitment);
    block.vtx[0] = MakeTransactionRef(std::move(mtx));

    BlockValidationState state;

    // The coinbase commits to witness data but carries no reserved value.
    BOOST_CHECK(!ContextualCheckBlock(block, state, consensusParams, &prev));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-witness-nonce-size");
}

BOOST_AUTO_TEST_CASE(contextual_block_bad_witness_merkle_match)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    CBlockIndex prev;
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_SEGWIT);
    prev.nTime = block.nTime;
    const int height{prev.nHeight + 1};

    // As above, but with a reserved value of the expected size, so that the
    // commitment itself is compared against the witness merkle root.
    std::vector<unsigned char> commitment{0xaa, 0x21, 0xa9, 0xed};
    commitment.resize(36);
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vin[0].scriptSig = CScript() << height;
    mtx.vin[0].scriptWitness.stack.emplace_back(32, 0);
    mtx.vout.emplace_back(0, CScript() << OP_RETURN << commitment);
    block.vtx[0] = MakeTransactionRef(std::move(mtx));

    BlockValidationState state;

    // The commitment is all zeroes, which is not the witness merkle root.
    BOOST_CHECK(!ContextualCheckBlock(block, state, consensusParams, &prev));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-witness-merkle-match");
}

BOOST_AUTO_TEST_CASE(contextual_block_bad_blk_weight)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};
    CBlock block{chainparams->GenesisBlock()};

    // Pad the coinbase with the maximum base size worth of data. There is no
    // witness data, so the weight is four times the base size and the block
    // ends up just over the limit.
    const std::vector<unsigned char> padding(MAX_BLOCK_WEIGHT / WITNESS_SCALE_FACTOR, OP_RETURN);
    CMutableTransaction mtx{*block.vtx[0]};
    mtx.vout[0].scriptPubKey = CScript(padding.begin(), padding.end());
    block.vtx[0] = MakeTransactionRef(std::move(mtx));

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlock(block, state, consensusParams, /*pindexPrev=*/nullptr));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-blk-weight");
}

BOOST_AUTO_TEST_SUITE_END()
