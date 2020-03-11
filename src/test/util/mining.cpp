// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/mining.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <miner.h>
#include <node/context.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <validation.h>
#include <validationinterface.h>

COutPoint generatetoaddress(const NodeContext& node, const std::string& address)
{
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    const auto coinbase_script = GetScriptForDestination(dest);

    return MineBlock(node, coinbase_script);
}

void ReGenerateCommitments(CBlock& block)
{
    CMutableTransaction tx{*block.vtx.at(0)};
    tx.vout.erase(tx.vout.begin() + GetWitnessCommitmentIndex(block));
    block.vtx.at(0) = MakeTransactionRef(tx);

    GenerateCoinbaseCommitment(block, WITH_LOCK(cs_main, return LookupBlockIndex(block.hashPrevBlock)), Params().GetConsensus());

    block.hashMerkleRoot = BlockMerkleRoot(block);
}

COutPoint MineBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    auto block = PrepareBlock(node, coinbase_scriptPubKey);
    auto valid = MineBlock(block);
    assert(!valid.IsNull());
    return valid;
}

struct BlockValidationStateCatcher : public CValidationInterface {
    const uint256 m_hash;
    Optional<BlockValidationState> m_state;

    BlockValidationStateCatcher(const uint256& hash)
        : m_hash{hash},
          m_state{} {}

protected:
    void BlockChecked(const CBlock& block, const BlockValidationState& state) override
    {
        if (block.GetHash() != m_hash) return;
        m_state = state;
    }
};

COutPoint MineBlock(std::shared_ptr<CBlock>& block)
{
    while (!CheckProofOfWork(block->GetHash(), block->nBits, Params().GetConsensus())) {
        ++block->nNonce;
        assert(block->nNonce);
    }

    const auto old_height = WITH_LOCK(cs_main, return ChainActive().Height());
    bool new_block;
    BlockValidationStateCatcher bvsc{block->GetHash()};
    RegisterValidationInterface(&bvsc);
    const bool processed{ProcessNewBlock(Params(), block, /* fForceProcessing */ true, /* fNewBlock */ &new_block)};
    const bool duplicate{!new_block && processed};
    assert(!duplicate);
    UnregisterValidationInterface(&bvsc);
    SyncWithValidationInterfaceQueue();
    const bool was_valid{bvsc.m_state && bvsc.m_state->IsValid()};
    assert(old_height + was_valid == WITH_LOCK(cs_main, return ChainActive().Height()));

    if (was_valid) return {block->vtx[0]->GetHash(), 0};
    return {};
}

std::shared_ptr<CBlock> PrepareBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    assert(node.mempool);
    auto block = std::make_shared<CBlock>(
        BlockAssembler{*node.mempool, Params()}
            .CreateNewBlock(coinbase_scriptPubKey)
            ->block);

    LOCK(cs_main);
    block->nTime = ::ChainActive().Tip()->GetMedianTimePast() + 1;
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    return block;
}
