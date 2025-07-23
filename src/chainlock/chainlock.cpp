// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainlock/chainlock.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <node/interface_ui.h>
#include <scheduler.h>
#include <txmempool.h>
#include <util/thread.h>
#include <util/time.h>
#include <util/underlying.h>
#include <validation.h>
#include <validationinterface.h>

#include <chainlock/signing.h>
#include <instantsend/instantsend.h>
#include <llmq/quorums.h>
#include <masternode/sync.h>
#include <spork.h>
#include <stats/client.h>

// Forward declaration to break dependency over node/transaction.h
namespace node {
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool,
                               const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock);
} // namespace node

using node::GetTransaction;

namespace llmq {
namespace {
static constexpr int64_t CLEANUP_INTERVAL{1000 * 30};
static constexpr int64_t CLEANUP_SEEN_TIMEOUT{24 * 60 * 60 * 1000};
//! How long to wait for islocks until we consider a block with non-islocked TXs to be safe to sign
static constexpr int64_t WAIT_FOR_ISLOCK_TIMEOUT{10 * 60};
} // anonymous namespace

bool AreChainLocksEnabled(const CSporkManager& sporkman)
{
    return sporkman.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED);
}

CChainLocksHandler::CChainLocksHandler(CChainState& chainstate, CQuorumManager& _qman, CSigningManager& _sigman,
                                       CSigSharesManager& _shareman, CSporkManager& sporkman, CTxMemPool& _mempool,
                                       const CMasternodeSync& mn_sync, bool is_masternode) :
    m_chainstate{chainstate},
    qman{_qman},
    spork_manager{sporkman},
    mempool{_mempool},
    m_mn_sync{mn_sync},
    scheduler{std::make_unique<CScheduler>()},
    scheduler_thread{
        std::make_unique<std::thread>(std::thread(util::TraceThread, "cl-schdlr", [&] { scheduler->serviceQueue(); }))},
    m_signer{is_masternode
                 ? std::make_unique<chainlock::ChainLockSigner>(chainstate, *this, _sigman, _shareman, sporkman, mn_sync)
                 : nullptr}
{
}

CChainLocksHandler::~CChainLocksHandler()
{
    scheduler->stop();
    scheduler_thread->join();
}

void CChainLocksHandler::Start(const llmq::CInstantSendManager& isman)
{
    if (m_signer) {
        m_signer->Start();
    }
    scheduler->scheduleEvery([&]() {
        CheckActiveState();
        EnforceBestChainLock();
        Cleanup();
        // regularly retry signing the current chaintip as it might have failed before due to missing islocks
        if (m_signer) {
            m_signer->TrySignChainTip(isman);
        }
    }, std::chrono::seconds{5});
}

void CChainLocksHandler::Stop()
{
    scheduler->stop();
    if (m_signer) {
        m_signer->Stop();
    }
}

bool CChainLocksHandler::AlreadyHave(const CInv& inv) const
{
    LOCK(cs);
    return seenChainLocks.count(inv.hash) != 0;
}

bool CChainLocksHandler::GetChainLockByHash(const uint256& hash, chainlock::ChainLockSig& ret) const
{
    LOCK(cs);

    if (hash != bestChainLockHash) {
        // we only propagate the best one and ditch all the old ones
        return false;
    }

    ret = bestChainLock;
    return true;
}

chainlock::ChainLockSig CChainLocksHandler::GetBestChainLock() const
{
    LOCK(cs);
    return bestChainLock;
}

void CChainLocksHandler::UpdateTxFirstSeenMap(const std::unordered_set<uint256, StaticSaltedHasher>& tx, const int64_t& time)
{
    AssertLockNotHeld(cs);
    LOCK(cs);
    for (const auto& txid : tx) {
        txFirstSeenTime.emplace(txid, time);
    }
}

MessageProcessingResult CChainLocksHandler::ProcessNewChainLock(const NodeId from, const chainlock::ChainLockSig& clsig,
                                                                const uint256& hash)
{
    CheckActiveState();

    {
        LOCK(cs);
        if (!seenChainLocks.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now())).second) {
            return {};
        }

        if (!bestChainLock.IsNull() && clsig.getHeight() <= bestChainLock.getHeight()) {
            // no need to process/relay older CLSIGs
            return {};
        }
    }

    if (const auto ret = VerifyChainLock(clsig); ret != VerifyRecSigStatus::Valid) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), status=%d peer=%d\n", __func__, clsig.ToString(), ToUnderlying(ret), from);
        if (from != -1) {
            return MisbehavingError{10};
        }
        return {};
    }

    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(clsig.getBlockHash()));

    {
        LOCK(cs);
        bestChainLockHash = hash;
        bestChainLock = clsig;

        if (pindex != nullptr) {

            if (pindex->nHeight != clsig.getHeight()) {
                // Should not happen, same as the conflict check from above.
                LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                        __func__, clsig.ToString(), pindex->nHeight);
                // Note: not relaying clsig here
                return {};
            }

            bestChainLockWithKnownBlock = bestChainLock;
            bestChainLockBlockIndex = pindex;
        }
        // else if (pindex == nullptr)
        // Note: make sure to still relay clsig further.
    }

    CInv clsigInv(MSG_CLSIG, hash);

    if (pindex == nullptr) {
        // we don't know the block/header for this CLSIG yet, so bail out for now
        // when the block or the header later comes in, we will enforce the correct chain
        return clsigInv;
    }

    scheduler->scheduleFromNow([&]() {
        CheckActiveState();
        EnforceBestChainLock();
    }, std::chrono::seconds{0});

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- processed new CLSIG (%s), peer=%d\n",
              __func__, clsig.ToString(), from);
    return clsigInv;
}

void CChainLocksHandler::AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew)
{
    LOCK(cs);

    if (pindexNew->GetBlockHash() == bestChainLock.getBlockHash()) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- block header %s came in late, updating and enforcing\n", __func__, pindexNew->GetBlockHash().ToString());

        if (bestChainLock.getHeight() != pindexNew->nHeight) {
            // Should not happen, same as the conflict check from ProcessNewChainLock.
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                      __func__, bestChainLock.ToString(), pindexNew->nHeight);
            return;
        }

        // when EnforceBestChainLock is called later, it might end up invalidating other chains but not activating the
        // CLSIG locked chain. This happens when only the header is known but the block is still missing yet. The usual
        // block processing logic will handle this when the block arrives
        bestChainLockWithKnownBlock = bestChainLock;
        bestChainLockBlockIndex = pindexNew;
    }
}

void CChainLocksHandler::UpdatedBlockTip(const llmq::CInstantSendManager& isman)
{
    // don't call TrySignChainTip directly but instead let the scheduler call it. This way we ensure that cs_main is
    // never locked and TrySignChainTip is not called twice in parallel. Also avoids recursive calls due to
    // EnforceBestChainLock switching chains.
    // atomic[If tryLockChainTipScheduled is false, do (set it to true] and schedule signing).
    if (bool expected = false; tryLockChainTipScheduled.compare_exchange_strong(expected, true)) {
        scheduler->scheduleFromNow([&]() {
            CheckActiveState();
            EnforceBestChainLock();
            Cleanup();
            if (m_signer) {
                m_signer->TrySignChainTip(isman);
            }
            tryLockChainTipScheduled = false;
        }, std::chrono::seconds{0});
    }
}

void CChainLocksHandler::CheckActiveState()
{
    bool oldIsEnabled = isEnabled;
    isEnabled = AreChainLocksEnabled(spork_manager);

    if (!oldIsEnabled && isEnabled) {
        // ChainLocks got activated just recently, but it's possible that it was already running before, leaving
        // us with some stale values which we should not try to enforce anymore (there probably was a good reason
        // to disable spork19)
        LOCK(cs);
        bestChainLockHash = uint256();
        bestChainLock = bestChainLockWithKnownBlock = chainlock::ChainLockSig();
        bestChainLockBlockIndex = lastNotifyChainLockBlockIndex = nullptr;
    }
}

void CChainLocksHandler::TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime)
{
    if (tx->IsCoinBase() || tx->vin.empty()) {
        return;
    }

    LOCK(cs);
    txFirstSeenTime.emplace(tx->GetHash(), nAcceptTime);
}

void CChainLocksHandler::BlockConnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindex)
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

    // We need this information later when we try to sign a new tip, so that we can determine if all included TXs are safe.
    if (m_signer) {
        m_signer->UpdateBlockHashTxidMap(pindex->GetBlockHash(), pblock->vtx);
    }
}

void CChainLocksHandler::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected)
{
    if (m_signer) {
        m_signer->EraseFromBlockHashTxidMap(pindexDisconnected->GetBlockHash());
    }
}

int32_t CChainLocksHandler::GetBestChainLockHeight() const
{
    AssertLockNotHeld(cs);
    LOCK(cs);
    return bestChainLock.getHeight();
}

bool CChainLocksHandler::IsTxSafeForMining(const uint256& txid) const
{
    int64_t txAge = 0;
    {
        LOCK(cs);
        auto it = txFirstSeenTime.find(txid);
        if (it != txFirstSeenTime.end()) {
            txAge = GetTime<std::chrono::seconds>().count() - it->second;
        }
    }

    return txAge >= WAIT_FOR_ISLOCK_TIMEOUT;
}

// WARNING: cs_main and cs should not be held!
// This should also not be called from validation signals, as this might result in recursive calls
void CChainLocksHandler::EnforceBestChainLock()
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_main);

    std::shared_ptr<chainlock::ChainLockSig> clsig;
    const CBlockIndex* pindex;
    const CBlockIndex* currentBestChainLockBlockIndex;
    {
        LOCK(cs);

        if (!IsEnabled()) {
            return;
        }

        clsig = std::make_shared<chainlock::ChainLockSig>(bestChainLockWithKnownBlock);
        pindex = currentBestChainLockBlockIndex = this->bestChainLockBlockIndex;

        if (currentBestChainLockBlockIndex == nullptr) {
            // we don't have the header/block, so we can't do anything right now
            return;
        }
    }

    BlockValidationState dummy_state;

    // Go backwards through the chain referenced by clsig until we find a block that is part of the main chain.
    // For each of these blocks, check if there are children that are NOT part of the chain referenced by clsig
    // and mark all of them as conflicting.
    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- enforcing block %s via CLSIG (%s)\n", __func__, pindex->GetBlockHash().ToString(), clsig->ToString());
    m_chainstate.EnforceBlock(dummy_state, pindex);


    if (/*activateNeeded =*/ WITH_LOCK(::cs_main, return m_chainstate.m_chain.Tip()->GetAncestor(currentBestChainLockBlockIndex->nHeight)) != currentBestChainLockBlockIndex) {
        if (!m_chainstate.ActivateBestChain(dummy_state)) {
            LogPrintf("CChainLocksHandler::%s -- ActivateBestChain failed: %s\n", __func__, dummy_state.ToString());
            return;
        }
        LOCK(::cs_main);
        if (m_chainstate.m_chain.Tip()->GetAncestor(currentBestChainLockBlockIndex->nHeight) != currentBestChainLockBlockIndex) {
            return;
        }
    }

    {
        LOCK(cs);
        if (lastNotifyChainLockBlockIndex == currentBestChainLockBlockIndex) return;
        lastNotifyChainLockBlockIndex = currentBestChainLockBlockIndex;
    }

    GetMainSignals().NotifyChainLock(currentBestChainLockBlockIndex, clsig, clsig->ToString());
    uiInterface.NotifyChainLock(clsig->getBlockHash().ToString(), clsig->getHeight());
    ::g_stats_client->gauge("chainlocks.blockHeight", clsig->getHeight(), 1.0f);
}

VerifyRecSigStatus CChainLocksHandler::VerifyChainLock(const chainlock::ChainLockSig& clsig) const
{
    const auto llmqType = Params().GetConsensus().llmqTypeChainLocks;
    const uint256 nRequestId = ::SerializeHash(std::make_pair(chainlock::CLSIG_REQUESTID_PREFIX, clsig.getHeight()));

    return llmq::VerifyRecoveredSig(llmqType, m_chainstate.m_chain, qman, clsig.getHeight(), nRequestId, clsig.getBlockHash(), clsig.getSig());
}

bool CChainLocksHandler::HasChainLock(int nHeight, const uint256& blockHash) const
{
    AssertLockNotHeld(cs);
    LOCK(cs);

    if (!IsEnabled()) {
        return false;
    }

    if (bestChainLockBlockIndex == nullptr) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash == bestChainLockBlockIndex->GetBlockHash();
    }

    const auto* pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    return (pAncestor != nullptr) && pAncestor->GetBlockHash() == blockHash;
}

bool CChainLocksHandler::HasConflictingChainLock(int nHeight, const uint256& blockHash) const
{
    AssertLockNotHeld(cs);
    LOCK(cs);

    if (!IsEnabled()) {
        return false;
    }

    if (bestChainLockBlockIndex == nullptr) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash != bestChainLockBlockIndex->GetBlockHash();
    }

    const auto* pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    assert(pAncestor);
    return pAncestor->GetBlockHash() != blockHash;
}

void CChainLocksHandler::Cleanup()
{
    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - lastCleanupTime < CLEANUP_INTERVAL) {
        return;
    }
    lastCleanupTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());

    {
        LOCK(cs);
        for (auto it = seenChainLocks.begin(); it != seenChainLocks.end(); ) {
            if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - it->second >= CLEANUP_SEEN_TIMEOUT) {
                it = seenChainLocks.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (m_signer) {
        const auto cleanup_txes{m_signer->Cleanup()};
        LOCK(cs);
        for (const auto& tx : cleanup_txes) {
            for (const auto& txid : *tx) {
                txFirstSeenTime.erase(txid);
            }
        }
    }

    LOCK(::cs_main);
    LOCK2(mempool.cs, cs); // need mempool.cs due to GetTransaction calls
    for (auto it = txFirstSeenTime.begin(); it != txFirstSeenTime.end(); ) {
        uint256 hashBlock;
        if (auto tx = GetTransaction(nullptr, &mempool, it->first, Params().GetConsensus(), hashBlock); !tx) {
            // tx has vanished, probably due to conflicts
            it = txFirstSeenTime.erase(it);
        } else if (!hashBlock.IsNull()) {
            const auto* pindex = m_chainstate.m_blockman.LookupBlockIndex(hashBlock);
            if (m_chainstate.m_chain.Tip()->GetAncestor(pindex->nHeight) == pindex && m_chainstate.m_chain.Height() - pindex->nHeight >= 6) {
                // tx got confirmed >= 6 times, so we can stop keeping track of it
                it = txFirstSeenTime.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}
} // namespace llmq
