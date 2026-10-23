// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <node/tx_collection.h>

#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <node/blockstorage.h>
#include <node/miner.h>
#include <node/mining_types.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/hasher.h>
#include <util/time.h>
#include <validation.h>
#include <versionbits.h>

#include <boost/multi_index/detail/hash_index_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace node {

TxCollection::TxCollection(std::vector<Wtxid> wtxids, ChainstateManager& chainman, CTxMemPool& mempool)
    : m_wtxids(std::move(wtxids)),
      m_chainman(chainman),
      m_mempool(mempool)
{
    // Check for excessively high numbers of transactions.
    if (m_wtxids.size() > MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT) {
        throw std::runtime_error(strprintf("too many wtxids (%d > %d)", m_wtxids.size(), MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT));
    }
    m_transactions.reserve(m_wtxids.size());
    for (const auto& wtxid : m_wtxids) {
        if (!m_transactions.emplace(wtxid, nullptr).second) {
            throw std::runtime_error(strprintf("duplicate wtxid %s", wtxid.ToString()));
        }
    }
    LOCK(m_mempool.cs);
    for (auto& [wtxid, tx] : m_transactions) {
        if (const auto it{m_mempool.GetIter(wtxid)}) {
            tx = (*it)->GetSharedTx();
        }
    }
}

std::vector<uint32_t> TxCollection::UnknownTxPos() const
{
    LOCK(m_mutex);
    std::vector<uint32_t> result;
    for (size_t i{0}; i < m_wtxids.size(); ++i) {
        // Every requested wtxid is a key (added in the constructor), so at()
        // is safe; a null value means the transaction is still missing.
        if (!m_transactions.at(m_wtxids[i])) result.push_back(static_cast<uint32_t>(i));
    }
    return result;
}

void TxCollection::AddMissingTxs(const std::vector<CTransactionRef>& txs)
{
    LOCK(m_mutex);
    // Reject a list with more transactions than the whole collection.
    // Ideally the IPC layer would enforce a limit based on the maximum block
    // size before deserializing the transactions, but it currently cannot.
    if (txs.size() > m_transactions.size()) {
        throw std::runtime_error(strprintf("too many transactions (%d > %d)", txs.size(), m_transactions.size()));
    }
    // Check for null entries and unexpected or duplicate wtxids before adding any
    // transaction, so a failed call leaves the collection unchanged.
    std::unordered_set<Wtxid, SaltedWtxidHasher> seen;
    seen.reserve(txs.size());
    for (const auto& tx : txs) {
        if (!tx) throw std::runtime_error("unexpected null transaction");
        const auto& wtxid{tx->GetWitnessHash()};
        if (!m_transactions.contains(wtxid)) {
            throw std::runtime_error(strprintf("unexpected wtxid %s", wtxid.ToString()));
        }
        if (!seen.insert(wtxid).second) {
            throw std::runtime_error(strprintf("duplicate wtxid %s", wtxid.ToString()));
        }
    }
    for (const auto& tx : txs) {
        auto& entry{m_transactions.at(tx->GetWitnessHash())};
        if (!entry) entry = tx;
    }
}

std::unique_ptr<CBlockTemplate> TxCollection::MakeTemplate(const uint256& prevhash,
                                                           const CTransactionRef& coinbase,
                                                           std::string& reason,
                                                           std::string& debug)
{
    reason.clear();
    debug.clear();

    auto block_template{[&]() -> std::unique_ptr<CBlockTemplate> {
        std::vector<CTransactionRef> transactions;
        {
            LOCK(m_mutex);
            if (std::ranges::any_of(m_transactions, [](const auto& entry) { return !entry.second; })) {
                reason = "missing-txs";
                debug = "collected transaction(s) still missing";
                return nullptr;
            }
            transactions.reserve(m_wtxids.size());
            for (const auto& wtxid : m_wtxids) {
                const auto it{m_transactions.find(wtxid)};
                Assert(it != m_transactions.end());
                Assert(it->second);
                transactions.push_back(it->second);
            }
        }
        ChainstateManager& chainman{m_chainman};
        LOCK(chainman.GetMutex());
        const CBlockIndex* current_tip{chainman.ActiveTip()};
        if (!current_tip || prevhash != current_tip->GetBlockHash()) {
            reason = "inconclusive-not-best-prevblk";
            debug = strprintf("requested prevhash %s does not match current tip %s",
                              prevhash.ToString(),
                              current_tip ? current_tip->GetBlockHash().ToString() : "(none)");
            return nullptr;
        }
        const CBlockIndex* const prev_block{Assert(chainman.m_blockman.LookupBlockIndex(prevhash))};
        auto block_template{std::make_unique<CBlockTemplate>()};
        CBlock& block{block_template->block};

        block.hashPrevBlock = prevhash;
        block.nVersion = chainman.m_versionbitscache.ComputeBlockVersion(prev_block, chainman.GetParams().GetConsensus());
        if (chainman.GetParams().MineBlocksOnDemand()) {
            block.nVersion = gArgs.GetIntArg("-blockversion", block.nVersion);
        }
        block.nTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());

        // Placeholder for the coinbase tx, filled in after the collected
        // transactions are added so the witness commitment is current.
        block.vtx.emplace_back();

        for (const auto& tx : transactions) {
            block.vtx.push_back(tx);
        }

        if (coinbase) {
            // Validate the block with the caller-provided coinbase, which is
            // expected to commit to the collected transactions (witness
            // commitment) and the next block height (BIP34).
            block.vtx[0] = coinbase;
        } else {
            // Validate with a node-generated dummy coinbase. Checks involving the
            // coinbase still pass, but say nothing about the coinbase the caller
            // intends to use.
            BlockAssembler{
                chainman.ActiveChainstate(),
                &m_mempool,
                BlockCreateOptions{.use_mempool = false},
            }
                .CreateCoinbaseTx(block, *prev_block, /*fees=*/0);
        }

        UpdateTime(&block, chainman.GetParams().GetConsensus(), prev_block);
        block.nBits = GetNextWorkRequired(prev_block, &block, chainman.GetParams().GetConsensus());
        block.nNonce = 0;

        BlockValidationState state{TestBlockValidity(chainman.ActiveChainstate(), block, /*check_pow=*/false, /*check_merkle_root=*/false)};
        if (!state.IsValid()) {
            reason = state.GetRejectReason();
            debug = state.GetDebugMessage();
            return nullptr;
        }
        return block_template;
    }()};
    // This follows the SubmitBlock convention: a missing template must set a
    // rejection reason, and a successful one must not.
    CHECK_NONFATAL((block_template != nullptr) == reason.empty());
    return block_template;
}
} // namespace node
