// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainlock/handler.h>

#include <chain.h>
#include <chainlock/chainlock.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <instantsend/instantsend.h>
#include <llmq/quorumsman.h>
#include <masternode/sync.h>
#include <node/interface_ui.h>
#include <scheduler.h>
#include <stats/client.h>
#include <txmempool.h>
#include <util/thread.h>
#include <util/time.h>
#include <util/underlying.h>
#include <validation.h>
#include <validationinterface.h>

// Forward declaration to break dependency over node/transaction.h
namespace node {
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool,
                               const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock);
} // namespace node

using node::GetTransaction;

namespace {
static constexpr auto CLEANUP_SEEN_TIMEOUT{24h};
//! How long to wait for islocks until we consider a block with non-islocked TXs to be safe to sign
static constexpr auto WAIT_FOR_ISLOCK_TIMEOUT{10min};
} // anonymous namespace

namespace chainlock {
ChainlockHandler::ChainlockHandler(chainlock::Chainlocks& chainlocks, ChainstateManager& chainman, CTxMemPool& _mempool,
                                   const CMasternodeSync& mn_sync) :
    m_chainlocks{chainlocks},
    m_chainman{chainman},
    mempool{_mempool},
    m_mn_sync{mn_sync},
    scheduler{std::make_unique<CScheduler>()},
    scheduler_thread{
        std::make_unique<std::thread>(std::thread(util::TraceThread, "cl-schdlr", [&] { scheduler->serviceQueue(); }))}
{
}

ChainlockHandler::~ChainlockHandler()
{
    scheduler->stop();
    scheduler_thread->join();
}

void ChainlockHandler::Start()
{
    scheduler->scheduleEvery(
        [&]() {
            CheckActiveState();
            EnforceBestChainLock();
            Cleanup();
        },
        std::chrono::seconds{5});
}

void ChainlockHandler::Stop() { scheduler->stop(); }

bool ChainlockHandler::AlreadyHave(const CInv& inv) const
{
    LOCK(cs);
    return seenChainLocks.count(inv.hash) != 0;
}

void ChainlockHandler::UpdateTxFirstSeenMap(const Uint256HashSet& tx, const int64_t& time)
{
    AssertLockNotHeld(cs);
    LOCK(cs);
    for (const auto& txid : tx) {
        txFirstSeenTime.emplace(txid, time);
    }
}

MessageProcessingResult ChainlockHandler::ProcessNewChainLock(const NodeId from, const chainlock::ChainLockSig& clsig,
                                                              const llmq::CQuorumManager& qman, const uint256& hash)
{
    CheckActiveState();

    {
        LOCK(cs);
        if (!seenChainLocks.emplace(hash, GetTime<std::chrono::seconds>()).second) {
            return {};
        }

        // height is expect to check twice: preliminary (for optimization) and inside UpdateBestsChainlock (as mutex is not kept during validation)
        if (clsig.getHeight() <= m_chainlocks.GetBestChainLockHeight()) {
            // no need to process older/same CLSIGs
            return {};
        }
    }

    if (const auto ret = chainlock::VerifyChainLock(Params().GetConsensus(), m_chainman.ActiveChain(), qman, clsig);
        ret != llmq::VerifyRecSigStatus::Valid) {
        LogPrint(BCLog::CHAINLOCKS, "ChainlockHandler::%s -- invalid CLSIG (%s), status=%d peer=%d\n", __func__,
                 clsig.ToString(), ToUnderlying(ret), from);
        if (from != -1) {
            return MisbehavingError{10};
        }
        return {};
    }

    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return m_chainman.m_blockman.LookupBlockIndex(clsig.getBlockHash()));

    if (!m_chainlocks.UpdateBestChainlock(hash, clsig, pindex)) return {};

    if (pindex) {
        scheduler->scheduleFromNow(
            [&]() {
                CheckActiveState();
                EnforceBestChainLock();
            },
            std::chrono::seconds{0});

        LogPrint(BCLog::CHAINLOCKS, "ChainlockHandler::%s -- processed new CLSIG (%s), peer=%d\n", __func__,
                 clsig.ToString(), from);
    }
    const CInv clsig_inv(MSG_CLSIG, hash);
    return clsig_inv;
}

void ChainlockHandler::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;
    if (fInitialDownload) return;

    // TODO: reconsider removing scheduler from here, because UpdatedBlockTip is already async call from notification
    // scheduler don't call TrySignChainTip directly but instead let the scheduler call it. This way we ensure that
    // cs_main is never locked and TrySignChainTip is not called twice in parallel. Also avoids recursive calls due to
    // EnforceBestChainLock switching chains.
    // atomic[If tryLockChainTipScheduled is false, do (set it to true] and schedule signing).

    if (bool expected = false; tryLockChainTipScheduled.compare_exchange_strong(expected, true)) {
        scheduler->scheduleFromNow(
            [&]() {
                CheckActiveState();
                EnforceBestChainLock();
                Cleanup();
                tryLockChainTipScheduled = false;
            },
            std::chrono::seconds{0});
    }
}

void ChainlockHandler::CheckActiveState()
{
    bool oldIsEnabled = isEnabled;
    isEnabled = m_chainlocks.IsEnabled();

    if (!oldIsEnabled && isEnabled) {
        // ChainLocks got activated just recently, but it's possible that it was already running before, leaving
        // us with some stale values which we should not try to enforce anymore (there probably was a good reason
        // to disable spork19)
        LOCK(cs);
        m_chainlocks.ResetChainlock();
        lastNotifyChainLockBlockIndex = nullptr;
    }
}

void ChainlockHandler::TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime, uint64_t)
{
    if (tx->IsCoinBase() || tx->vin.empty()) {
        return;
    }

    LOCK(cs);
    txFirstSeenTime.emplace(tx->GetHash(), nAcceptTime);
}

void ChainlockHandler::AcceptedBlockHeader(const CBlockIndex* pindexNew)
{
    m_chainlocks.AcceptedBlockHeader(pindexNew);
}

void ChainlockHandler::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    // We listen for BlockConnected so that we can collect all TX ids of all included TXs of newly received blocks
    int64_t curTime = GetTime<std::chrono::seconds>().count();
    {
        LOCK(cs);
        for (const auto& tx : pblock->vtx) {
            if (!tx->IsCoinBase() && !tx->vin.empty()) {
                txFirstSeenTime.emplace(tx->GetHash(), curTime);
            }
        }
    }
}

bool ChainlockHandler::IsTxSafeForMining(const uint256& txid) const
{
    auto tx_age{0s};
    {
        LOCK(cs);
        auto it = txFirstSeenTime.find(txid);
        if (it != txFirstSeenTime.end()) {
            tx_age = GetTime<std::chrono::seconds>() - it->second;
        }
    }

    return tx_age >= WAIT_FOR_ISLOCK_TIMEOUT;
}

// WARNING: cs_main and cs should not be held!
// This should also not be called from validation signals, as this might result in recursive calls
void ChainlockHandler::EnforceBestChainLock()
{
    if (!isEnabled) {
        return;
    }

    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_main);


    auto [clsig, currentBestChainLockBlockIndex] = m_chainlocks.GetBestChainlockWithPindex();
    if (currentBestChainLockBlockIndex == nullptr) {
        // we don't have the header/block, so we can't do anything right now
        return;
    }

    BlockValidationState dummy_state;

    // Go backwards through the chain referenced by clsig until we find a block that is part of the main chain.
    // For each of these blocks, check if there are children that are NOT part of the chain referenced by clsig
    // and mark all of them as conflicting.
    LogPrint(BCLog::CHAINLOCKS, "ChainlockHandler::%s -- enforcing block %s via CLSIG (%s)\n", __func__,
             currentBestChainLockBlockIndex->GetBlockHash().ToString(), clsig.ToString());
    m_chainman.ActiveChainstate().EnforceBlock(dummy_state, currentBestChainLockBlockIndex);


    if (/*activateNeeded =*/WITH_LOCK(::cs_main, return m_chainman.ActiveTip()->GetAncestor(
                                                     currentBestChainLockBlockIndex->nHeight)) !=
        currentBestChainLockBlockIndex) {
        if (!m_chainman.ActiveChainstate().ActivateBestChain(dummy_state)) {
            LogPrintf("ChainlockHandler::%s -- ActivateBestChain failed: %s\n", __func__, dummy_state.ToString());
            return;
        }
        LOCK(::cs_main);
        if (m_chainman.ActiveTip()->GetAncestor(currentBestChainLockBlockIndex->nHeight) != currentBestChainLockBlockIndex) {
            return;
        }
    }

    {
        LOCK(cs);
        if (lastNotifyChainLockBlockIndex == currentBestChainLockBlockIndex) return;
        lastNotifyChainLockBlockIndex = currentBestChainLockBlockIndex;
    }

    GetMainSignals().NotifyChainLock(currentBestChainLockBlockIndex, std::make_shared<chainlock::ChainLockSig>(clsig),
                                     clsig.ToString());
    uiInterface.NotifyChainLock(clsig.getBlockHash().ToString(), clsig.getHeight());
    ::g_stats_client->gauge("chainlocks.blockHeight", clsig.getHeight(), 1.0f);
}

void ChainlockHandler::CleanupFromSigner(const std::vector<std::shared_ptr<Uint256HashSet>>& cleanup_txes)
{
    LOCK(cs);
    for (const auto& tx : cleanup_txes) {
        for (const uint256& txid : *tx) {
            txFirstSeenTime.erase(txid);
        }
    }
}

void ChainlockHandler::Cleanup()
{
    constexpr auto CLEANUP_INTERVAL{30s};
    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (!cleanupThrottler.TryCleanup(CLEANUP_INTERVAL)) {
        return;
    }

    {
        LOCK(cs);
        for (auto it = seenChainLocks.begin(); it != seenChainLocks.end();) {
            if (GetTime<std::chrono::seconds>() - it->second >= CLEANUP_SEEN_TIMEOUT) {
                it = seenChainLocks.erase(it);
            } else {
                ++it;
            }
        }
    }

    LOCK(::cs_main);
    LOCK2(mempool.cs, cs); // need mempool.cs due to GetTransaction calls
    for (auto it = txFirstSeenTime.begin(); it != txFirstSeenTime.end();) {
        uint256 hashBlock;
        if (auto tx = GetTransaction(nullptr, &mempool, it->first, Params().GetConsensus(), hashBlock); !tx) {
            // tx has vanished, probably due to conflicts
            it = txFirstSeenTime.erase(it);
        } else if (!hashBlock.IsNull()) {
            const auto* pindex = m_chainman.m_blockman.LookupBlockIndex(hashBlock);
            assert(pindex); // GetTransaction gave us that hashBlock, it should resolve to a valid block index
            if (m_chainman.ActiveTip()->GetAncestor(pindex->nHeight) == pindex &&
                m_chainman.ActiveChain().Height() - pindex->nHeight > chainlock::TX_CONFIRM_THRESHOLD) {
                // tx is sufficiently deep, we can stop tracking it
                it = txFirstSeenTime.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

llmq::VerifyRecSigStatus VerifyChainLock(const Consensus::Params& params, const CChain& chain,
                                         const llmq::CQuorumManager& qman, const chainlock::ChainLockSig& clsig)
{
    const auto llmqType = params.llmqTypeChainLocks;
    const uint256 request_id = chainlock::GenSigRequestId(clsig.getHeight());

    return llmq::VerifyRecoveredSig(llmqType, chain, qman, clsig.getHeight(), request_id, clsig.getBlockHash(),
                                    clsig.getSig());
}
} // namespace chainlock
