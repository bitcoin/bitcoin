// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockstorage.h>
#include <txmempool.h>
#include <validation.h>

#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>
#include <instantsend/instantsend.h>
#include <masternode/sync.h>
#include <spork.h>

using node::ReadBlockFromDisk;

namespace llmq {
static bool ChainLocksSigningEnabled(const CSporkManager& sporkman)
{
    return sporkman.GetSporkValue(SPORK_19_CHAINLOCKS_ENABLED) == 0;
}

void CChainLocksHandler::TrySignChainTip(const llmq::CInstantSendManager& isman)
{
    Cleanup();

    if (!m_is_masternode) {
        return;
    }

    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (!isEnabled) {
        return;
    }

    if (!ChainLocksSigningEnabled(spork_manager)) {
        return;
    }

    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return m_chainstate.m_chain.Tip());

    if (pindex->pprev == nullptr) {
        return;
    }

    // DIP8 defines a process called "Signing attempts" which should run before the CLSIG is finalized
    // To simplify the initial implementation, we skip this process and directly try to create a CLSIG
    // This will fail when multiple blocks compete, but we accept this for the initial implementation.
    // Later, we'll add the multiple attempts process.

    {
        LOCK(cs);

        if (pindex->nHeight == lastSignedHeight) {
            // already signed this one
            return;
        }

        if (bestChainLock.getHeight() >= pindex->nHeight) {
            // already got the same CLSIG or a better one
            return;
        }

        if (InternalHasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            // don't sign if another conflicting CLSIG is already present. EnforceBestChainLock will later enforce
            // the correct chain.
            return;
        }
    }

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- trying to sign %s, height=%d\n", __func__, pindex->GetBlockHash().ToString(), pindex->nHeight);

    // When the new IX system is activated, we only try to ChainLock blocks which include safe transactions. A TX is
    // considered safe when it is islocked or at least known since 10 minutes (from mempool or block). These checks are
    // performed for the tip (which we try to sign) and the previous 5 blocks. If a ChainLocked block is found on the
    // way down, we consider all TXs to be safe.
    if (isman.IsInstantSendEnabled() && isman.RejectConflictingBlocks()) {
        const auto* pindexWalk = pindex;
        while (pindexWalk != nullptr) {
            if (pindex->nHeight - pindexWalk->nHeight > 5) {
                // no need to check further down, 6 confs is safe to assume that TXs below this height won't be
                // islocked anymore if they aren't already
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- tip and previous 5 blocks all safe\n", __func__);
                break;
            }
            if (HasChainLock(pindexWalk->nHeight, pindexWalk->GetBlockHash())) {
                // we don't care about islocks for TXs that are ChainLocked already
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- chainlock at height %d\n", __func__, pindexWalk->nHeight);
                break;
            }

            auto txids = GetBlockTxs(pindexWalk->GetBlockHash());
            if (!txids) {
                pindexWalk = pindexWalk->pprev;
                continue;
            }

            for (const auto& txid : *txids) {
                int64_t txAge = 0;
                {
                    LOCK(cs);
                    auto it = txFirstSeenTime.find(txid);
                    if (it != txFirstSeenTime.end()) {
                        txAge = GetTime<std::chrono::seconds>().count() - it->second;
                    }
                }

                if (txAge < WAIT_FOR_ISLOCK_TIMEOUT && !isman.IsLocked(txid)) {
                    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- not signing block %s due to TX %s not being islocked and not old enough. age=%d\n", __func__,
                              pindexWalk->GetBlockHash().ToString(), txid.ToString(), txAge);
                    return;
                }
            }

            pindexWalk = pindexWalk->pprev;
        }
    }

    uint256 requestId = ::SerializeHash(std::make_pair(chainlock::CLSIG_REQUESTID_PREFIX, pindex->nHeight));
    uint256 msgHash = pindex->GetBlockHash();

    {
        LOCK(cs);
        if (bestChainLock.getHeight() >= pindex->nHeight) {
            // might have happened while we didn't hold cs
            return;
        }
        lastSignedHeight = pindex->nHeight;
        lastSignedRequestId = requestId;
        lastSignedMsgHash = msgHash;
    }

    sigman.AsyncSignIfMember(Params().GetConsensus().llmqTypeChainLocks, shareman, requestId, msgHash);
}

CChainLocksHandler::BlockTxs::mapped_type CChainLocksHandler::GetBlockTxs(const uint256& blockHash)
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_main);

    CChainLocksHandler::BlockTxs::mapped_type ret;

    {
        LOCK(cs);
        auto it = blockTxs.find(blockHash);
        if (it != blockTxs.end()) {
            ret = it->second;
        }
    }
    if (!ret) {
        // This should only happen when freshly started.
        // If running for some time, SyncTransaction should have been called before which fills blockTxs.
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- blockTxs for %s not found. Trying ReadBlockFromDisk\n", __func__,
                 blockHash.ToString());

        uint32_t blockTime;
        {
            LOCK(::cs_main);
            const auto* pindex = m_chainstate.m_blockman.LookupBlockIndex(blockHash);
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
                return nullptr;
            }

            ret = std::make_shared<std::unordered_set<uint256, StaticSaltedHasher>>();
            for (const auto& tx : block.vtx) {
                if (tx->IsCoinBase() || tx->vin.empty()) {
                    continue;
                }
                ret->emplace(tx->GetHash());
            }

            blockTime = block.nTime;
        }

        LOCK(cs);
        blockTxs.emplace(blockHash, ret);
        for (const auto& txid : *ret) {
            txFirstSeenTime.emplace(txid, blockTime);
        }
    }
    return ret;
}

MessageProcessingResult CChainLocksHandler::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    if (!isEnabled) {
        return {};
    }

    chainlock::ChainLockSig clsig;
    {
        LOCK(cs);

        if (recoveredSig.getId() != lastSignedRequestId || recoveredSig.getMsgHash() != lastSignedMsgHash) {
            // this is not what we signed, so lets not create a CLSIG for it
            return {};
        }
        if (bestChainLock.getHeight() >= lastSignedHeight) {
            // already got the same or a better CLSIG through the CLSIG message
            return {};
        }

        clsig = chainlock::ChainLockSig(lastSignedHeight, lastSignedMsgHash, recoveredSig.sig.Get());
    }
    return ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig));
}

std::vector<std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>> CChainLocksHandler::CleanupSigner()
{
    AssertLockHeld(cs);
    std::vector<std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>> removed;
    LOCK(::cs_main);
    for (auto it = blockTxs.begin(); it != blockTxs.end(); ) {
        const auto* pindex = m_chainstate.m_blockman.LookupBlockIndex(it->first);
        if (InternalHasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            removed.push_back(it->second);
            it = blockTxs.erase(it);
        } else if (InternalHasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            it = blockTxs.erase(it);
        } else {
            ++it;
        }
    }
    return removed;
}
} // namespace llmq
