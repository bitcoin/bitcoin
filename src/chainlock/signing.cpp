// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainlock/signing.h>

#include <node/blockstorage.h>
#include <validation.h>

#include <chainlock/clsig.h>
#include <instantsend/instantsend.h>
#include <masternode/sync.h>
#include <spork.h>

using node::ReadBlockFromDisk;

namespace chainlock {
ChainLockSigner::ChainLockSigner(CChainState& chainstate, ChainLockSignerParent& clhandler,
                                 llmq::CSigningManager& sigman, llmq::CSigSharesManager& shareman,
                                 CSporkManager& sporkman, const CMasternodeSync& mn_sync) :
    m_chainstate{chainstate},
    m_clhandler{clhandler},
    m_sigman{sigman},
    m_shareman{shareman},
    m_sporkman{sporkman},
    m_mn_sync{mn_sync}
{
}

ChainLockSigner::~ChainLockSigner() = default;

void ChainLockSigner::Start()
{
    m_sigman.RegisterRecoveredSigsListener(this);
}

void ChainLockSigner::Stop()
{
    m_sigman.UnregisterRecoveredSigsListener(this);
}

void ChainLockSigner::TrySignChainTip(const llmq::CInstantSendManager& isman)
{
    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (!m_clhandler.IsEnabled()) {
        return;
    }

    if (m_sporkman.GetSporkValue(SPORK_19_CHAINLOCKS_ENABLED) != 0) {
        // ChainLocks signing not enabled
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
        LOCK(cs_signer);

        if (pindex->nHeight == lastSignedHeight) {
            // already signed this one
            return;
        }
    }

    if (m_clhandler.GetBestChainLockHeight() >= pindex->nHeight) {
        // already got the same CLSIG or a better one
        return;
    }

    if (m_clhandler.HasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
        // don't sign if another conflicting CLSIG is already present. EnforceBestChainLock will later enforce
        // the correct chain.
        return;
    }

    LogPrint(BCLog::CHAINLOCKS, "%s -- trying to sign %s, height=%d\n", __func__, pindex->GetBlockHash().ToString(),
             pindex->nHeight);

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
                LogPrint(BCLog::CHAINLOCKS, "%s -- tip and previous 5 blocks all safe\n", __func__);
                break;
            }
            if (m_clhandler.HasChainLock(pindexWalk->nHeight, pindexWalk->GetBlockHash())) {
                // we don't care about islocks for TXs that are ChainLocked already
                LogPrint(BCLog::CHAINLOCKS, "%s -- chainlock at height %d\n", __func__, pindexWalk->nHeight);
                break;
            }

            auto txids = GetBlockTxs(pindexWalk->GetBlockHash());
            if (!txids) {
                pindexWalk = pindexWalk->pprev;
                continue;
            }

            for (const auto& txid : *txids) {
                if (!m_clhandler.IsTxSafeForMining(txid) && !isman.IsLocked(txid)) {
                    LogPrint(BCLog::CHAINLOCKS, /* Continued */
                             "%s -- not signing block %s due to TX %s not being islocked and not old enough.\n",
                             __func__, pindexWalk->GetBlockHash().ToString(), txid.ToString());
                    return;
                }
            }

            pindexWalk = pindexWalk->pprev;
        }
    }

    uint256 requestId = ::SerializeHash(std::make_pair(CLSIG_REQUESTID_PREFIX, pindex->nHeight));
    uint256 msgHash = pindex->GetBlockHash();

    if (m_clhandler.GetBestChainLockHeight() >= pindex->nHeight) {
        // might have happened while we didn't hold cs
        return;
    }
    {
        LOCK(cs_signer);
        lastSignedHeight = pindex->nHeight;
        lastSignedRequestId = requestId;
        lastSignedMsgHash = msgHash;
    }

    m_sigman.AsyncSignIfMember(Params().GetConsensus().llmqTypeChainLocks, m_shareman, requestId, msgHash);
}

void ChainLockSigner::EraseFromBlockHashTxidMap(const uint256& hash)
{
    AssertLockNotHeld(cs_signer);
    LOCK(cs_signer);
    blockTxs.erase(hash);
}

void ChainLockSigner::UpdateBlockHashTxidMap(const uint256& hash, const std::vector<CTransactionRef>& vtx)
{
    AssertLockNotHeld(cs_signer);
    LOCK(cs_signer);
    auto it = blockTxs.find(hash);
    if (it == blockTxs.end()) {
        // We must create this entry even if there are no lockable transactions in the block, so that TrySignChainTip
        // later knows about this block
        it = blockTxs.emplace(hash, std::make_shared<std::unordered_set<uint256, StaticSaltedHasher>>()).first;
    }
    auto& txids = *it->second;
    for (const auto& tx : vtx) {
        if (!tx->IsCoinBase() && !tx->vin.empty()) {
            txids.emplace(tx->GetHash());
        }
    }
}

ChainLockSigner::BlockTxs::mapped_type ChainLockSigner::GetBlockTxs(const uint256& blockHash)
{
    AssertLockNotHeld(cs_signer);
    AssertLockNotHeld(::cs_main);

    ChainLockSigner::BlockTxs::mapped_type ret;

    {
        LOCK(cs_signer);
        auto it = blockTxs.find(blockHash);
        if (it != blockTxs.end()) {
            ret = it->second;
        }
    }
    if (!ret) {
        // This should only happen when freshly started.
        // If running for some time, SyncTransaction should have been called before which fills blockTxs.
        LogPrint(BCLog::CHAINLOCKS, "%s -- blockTxs for %s not found. Trying ReadBlockFromDisk\n", __func__,
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
        {
            LOCK(cs_signer);
            blockTxs.emplace(blockHash, ret);
        }
        m_clhandler.UpdateTxFirstSeenMap(*ret, blockTime);
    }
    return ret;
}

MessageProcessingResult ChainLockSigner::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    if (!m_clhandler.IsEnabled()) {
        return {};
    }

    ChainLockSig clsig;
    {
        LOCK(cs_signer);

        if (recoveredSig.getId() != lastSignedRequestId || recoveredSig.getMsgHash() != lastSignedMsgHash) {
            // this is not what we signed, so lets not create a CLSIG for it
            return {};
        }
        if (m_clhandler.GetBestChainLockHeight() >= lastSignedHeight) {
            // already got the same or a better CLSIG through the CLSIG message
            return {};
        }

        clsig = ChainLockSig(lastSignedHeight, lastSignedMsgHash, recoveredSig.sig.Get());
    }
    return m_clhandler.ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig));
}

std::vector<std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>> ChainLockSigner::Cleanup()
{
    AssertLockNotHeld(cs_signer);
    std::vector<std::shared_ptr<std::unordered_set<uint256, StaticSaltedHasher>>> removed;
    LOCK2(::cs_main, cs_signer);
    for (auto it = blockTxs.begin(); it != blockTxs.end();) {
        const auto* pindex = m_chainstate.m_blockman.LookupBlockIndex(it->first);
        if (m_clhandler.HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            removed.push_back(it->second);
            it = blockTxs.erase(it);
        } else if (m_clhandler.HasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            it = blockTxs.erase(it);
        } else {
            ++it;
        }
    }
    return removed;
}
} // namespace chainlock
