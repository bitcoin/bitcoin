// Copyright (c) 2019-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/instantsend.h>

#include <llmq/chainlocks.h>
#include <llmq/quorums.h>
#include <llmq/utils.h>
#include <llmq/commitment.h>
#include <llmq/signing_shares.h>

#include <bls/bls_batchverifier.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <dbwrapper.h>
#include <index/txindex.h>
#include <masternode/sync.h>
#include <net_processing.h>
#include <spork.h>
#include <txmempool.h>
#include <util/irange.h>
#include <util/ranges.h>
#include <util/thread.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq
{

static const std::string INPUTLOCK_REQUESTID_PREFIX = "inlock";
static const std::string ISLOCK_REQUESTID_PREFIX = "islock";

static const std::string DB_ISLOCK_BY_HASH = "is_i";
static const std::string DB_HASH_BY_TXID = "is_tx";
static const std::string DB_HASH_BY_OUTPOINT = "is_in";
static const std::string DB_MINED_BY_HEIGHT_AND_HASH = "is_m";
static const std::string DB_ARCHIVED_BY_HEIGHT_AND_HASH = "is_a1";
static const std::string DB_ARCHIVED_BY_HASH = "is_a2";

static const std::string DB_VERSION = "is_v";

const int CInstantSendDb::CURRENT_VERSION;
const uint8_t CInstantSendLock::islock_version;
const uint8_t CInstantSendLock::isdlock_version;

std::unique_ptr<CInstantSendManager> quorumInstantSendManager;

uint256 CInstantSendLock::GetRequestId() const
{
    CHashWriter hw(SER_GETHASH, 0);
    hw << ISLOCK_REQUESTID_PREFIX;
    hw << inputs;
    return hw.GetHash();
}

////////////////


CInstantSendDb::CInstantSendDb(bool unitTests, bool fWipe) :
    db(std::make_unique<CDBWrapper>(unitTests ? "" : (GetDataDir() / "llmq/isdb"), 32 << 20, unitTests, fWipe))
{
}

CInstantSendDb::~CInstantSendDb() = default;

void CInstantSendDb::Upgrade(const CTxMemPool& mempool)
{
    LOCK2(cs_main, mempool.cs);
    LOCK(cs_db);
    int v{0};
    if (!db->Read(DB_VERSION, v) || v < CInstantSendDb::CURRENT_VERSION) {
        CDBBatch batch(*db);
        CInstantSendLock islock;

        auto it = std::unique_ptr<CDBIterator>(db->NewIterator());
        auto firstKey = std::make_tuple(DB_ISLOCK_BY_HASH, uint256());
        it->Seek(firstKey);
        decltype(firstKey) curKey;

        while (it->Valid()) {
            if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_ISLOCK_BY_HASH) {
                break;
            }
            uint256 hashBlock;
            CTransactionRef tx = GetTransaction(/* block_index */ nullptr, /* mempool */ nullptr, islock.txid, Params().GetConsensus(), hashBlock);
            if (it->GetValue(islock) && !tx) {
                // Drop locks for unknown txes
                batch.Erase(std::make_tuple(DB_HASH_BY_TXID, islock.txid));
                for (auto& in : islock.inputs) {
                    batch.Erase(std::make_tuple(DB_HASH_BY_OUTPOINT, in));
                }
                batch.Erase(curKey);
            }
            it->Next();
        }
        batch.Write(DB_VERSION, CInstantSendDb::CURRENT_VERSION);
        db->WriteBatch(batch);
    }
}

void CInstantSendDb::WriteNewInstantSendLock(const uint256& hash, const CInstantSendLock& islock)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    batch.Write(std::make_tuple(DB_ISLOCK_BY_HASH, hash), islock);
    batch.Write(std::make_tuple(DB_HASH_BY_TXID, islock.txid), hash);
    for (const auto& in : islock.inputs) {
        batch.Write(std::make_tuple(DB_HASH_BY_OUTPOINT, in), hash);
    }
    db->WriteBatch(batch);

    auto p = std::make_shared<CInstantSendLock>(islock);
    islockCache.insert(hash, p);
    txidCache.insert(islock.txid, hash);
    for (const auto& in : islock.inputs) {
        outpointCache.insert(in, hash);
    }
}

void CInstantSendDb::RemoveInstantSendLock(CDBBatch& batch, const uint256& hash, CInstantSendLockPtr islock, bool keep_cache)
{
    AssertLockHeld(cs_db);
    if (!islock) {
        islock = GetInstantSendLockByHashInternal(hash, false);
        if (!islock) {
            return;
        }
    }

    batch.Erase(std::make_tuple(DB_ISLOCK_BY_HASH, hash));
    batch.Erase(std::make_tuple(DB_HASH_BY_TXID, islock->txid));
    for (auto& in : islock->inputs) {
        batch.Erase(std::make_tuple(DB_HASH_BY_OUTPOINT, in));
    }

    if (!keep_cache) {
        islockCache.erase(hash);
        txidCache.erase(islock->txid);
        for (const auto& in : islock->inputs) {
            outpointCache.erase(in);
        }
    }
}

static std::tuple<std::string, uint32_t, uint256> BuildInversedISLockKey(const std::string& k, int nHeight, const uint256& islockHash)
{
    return std::make_tuple(k, htobe32(std::numeric_limits<uint32_t>::max() - nHeight), islockHash);
}

void CInstantSendDb::WriteInstantSendLockMined(const uint256& hash, int nHeight)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    WriteInstantSendLockMined(batch, hash, nHeight);
    db->WriteBatch(batch);
}

void CInstantSendDb::WriteInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight)
{
    AssertLockHeld(cs_db);
    batch.Write(BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nHeight, hash), true);
}

void CInstantSendDb::RemoveInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight)
{
    AssertLockHeld(cs_db);
    batch.Erase(BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nHeight, hash));
}

void CInstantSendDb::WriteInstantSendLockArchived(CDBBatch& batch, const uint256& hash, int nHeight)
{
    AssertLockHeld(cs_db);
    batch.Write(BuildInversedISLockKey(DB_ARCHIVED_BY_HEIGHT_AND_HASH, nHeight, hash), true);
    batch.Write(std::make_tuple(DB_ARCHIVED_BY_HASH, hash), true);
}

std::unordered_map<uint256, CInstantSendLockPtr, StaticSaltedHasher> CInstantSendDb::RemoveConfirmedInstantSendLocks(int nUntilHeight)
{
    LOCK(cs_db);
    if (nUntilHeight <= best_confirmed_height) {
        LogPrint(BCLog::ALL, "CInstantSendDb::%s -- Attempting to confirm height %d, however we've already confirmed height %d. This should never happen.\n", __func__,
                 nUntilHeight, best_confirmed_height);
        return {};
    }
    best_confirmed_height = nUntilHeight;

    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());

    auto firstKey = BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nUntilHeight, uint256());

    it->Seek(firstKey);

    CDBBatch batch(*db);
    std::unordered_map<uint256, CInstantSendLockPtr, StaticSaltedHasher> ret;
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_MINED_BY_HEIGHT_AND_HASH) {
            break;
        }
        uint32_t nHeight = std::numeric_limits<uint32_t>::max() - be32toh(std::get<1>(curKey));
        if (nHeight > uint32_t(nUntilHeight)) {
            break;
        }

        auto& islockHash = std::get<2>(curKey);
        auto islock = GetInstantSendLockByHashInternal(islockHash, false);
        if (islock) {
            RemoveInstantSendLock(batch, islockHash, islock);
            ret.emplace(islockHash, islock);
        }

        // archive the islock hash, so that we're still able to check if we've seen the islock in the past
        WriteInstantSendLockArchived(batch, islockHash, nHeight);

        batch.Erase(curKey);

        it->Next();
    }

    db->WriteBatch(batch);

    return ret;
}

void CInstantSendDb::RemoveArchivedInstantSendLocks(int nUntilHeight)
{
    LOCK(cs_db);
    if (nUntilHeight <= 0) {
        return;
    }

    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());

    auto firstKey = BuildInversedISLockKey(DB_ARCHIVED_BY_HEIGHT_AND_HASH, nUntilHeight, uint256());

    it->Seek(firstKey);

    CDBBatch batch(*db);
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_ARCHIVED_BY_HEIGHT_AND_HASH) {
            break;
        }
        uint32_t nHeight = std::numeric_limits<uint32_t>::max() - be32toh(std::get<1>(curKey));
        if (nHeight > uint32_t(nUntilHeight)) {
            break;
        }

        auto& islockHash = std::get<2>(curKey);
        batch.Erase(std::make_tuple(DB_ARCHIVED_BY_HASH, islockHash));
        batch.Erase(curKey);

        it->Next();
    }

    db->WriteBatch(batch);
}

void CInstantSendDb::WriteBlockInstantSendLocks(const gsl::not_null<std::shared_ptr<const CBlock>>& pblock,
                                                gsl::not_null<const CBlockIndex*> pindexConnected)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            // coinbase and TXs with no inputs can't be locked
            continue;
        }
        uint256 islockHash = GetInstantSendLockHashByTxidInternal(tx->GetHash());
        // update DB about when an IS lock was mined
        if (!islockHash.IsNull()) {
            WriteInstantSendLockMined(batch, islockHash, pindexConnected->nHeight);
        }
    }
    db->WriteBatch(batch);
}

void CInstantSendDb::RemoveBlockInstantSendLocks(const gsl::not_null<std::shared_ptr<const CBlock>>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected)
{
    LOCK(cs_db);
    CDBBatch batch(*db);
    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            // coinbase and TXs with no inputs can't be locked
            continue;
        }
        uint256 islockHash = GetInstantSendLockHashByTxidInternal(tx->GetHash());
        if (!islockHash.IsNull()) {
            RemoveInstantSendLockMined(batch, islockHash, pindexDisconnected->nHeight);
        }
    }
    db->WriteBatch(batch);
}

bool CInstantSendDb::KnownInstantSendLock(const uint256& islockHash) const
{
    LOCK(cs_db);
    return GetInstantSendLockByHashInternal(islockHash) != nullptr || db->Exists(std::make_tuple(DB_ARCHIVED_BY_HASH, islockHash));
}

size_t CInstantSendDb::GetInstantSendLockCount() const
{
    LOCK(cs_db);
    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());
    auto firstKey = std::make_tuple(DB_ISLOCK_BY_HASH, uint256());

    it->Seek(firstKey);

    size_t cnt = 0;
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_ISLOCK_BY_HASH) {
            break;
        }

        cnt++;

        it->Next();
    }

    return cnt;
}

CInstantSendLockPtr CInstantSendDb::GetInstantSendLockByHashInternal(const uint256& hash, bool use_cache) const
{
    AssertLockHeld(cs_db);
    if (hash.IsNull()) {
        return nullptr;
    }

    CInstantSendLockPtr ret;
    if (use_cache && islockCache.get(hash, ret)) {
        return ret;
    }

    ret = std::make_shared<CInstantSendLock>(CInstantSendLock::isdlock_version);
    bool exists = db->Read(std::make_tuple(DB_ISLOCK_BY_HASH, hash), *ret);
    if (!exists || (::SerializeHash(*ret) != hash)) {
        ret = std::make_shared<CInstantSendLock>();
        exists = db->Read(std::make_tuple(DB_ISLOCK_BY_HASH, hash), *ret);
        if (!exists || (::SerializeHash(*ret) != hash)) {
            ret = nullptr;
        }
    }
    islockCache.insert(hash, ret);
    return ret;
}

uint256 CInstantSendDb::GetInstantSendLockHashByTxidInternal(const uint256& txid) const
{
    AssertLockHeld(cs_db);
    uint256 islockHash;
    if (!txidCache.get(txid, islockHash)) {
        if (!db->Read(std::make_tuple(DB_HASH_BY_TXID, txid), islockHash)) {
            return {};
        }
        txidCache.insert(txid, islockHash);
    }
    return islockHash;
}

CInstantSendLockPtr CInstantSendDb::GetInstantSendLockByTxid(const uint256& txid) const
{
    LOCK(cs_db);
    return GetInstantSendLockByHashInternal(GetInstantSendLockHashByTxidInternal(txid));
}

CInstantSendLockPtr CInstantSendDb::GetInstantSendLockByInput(const COutPoint& outpoint) const
{
    LOCK(cs_db);
    uint256 islockHash;
    if (!outpointCache.get(outpoint, islockHash)) {
        if (!db->Read(std::make_tuple(DB_HASH_BY_OUTPOINT, outpoint), islockHash)) {
            return nullptr;
        }
        outpointCache.insert(outpoint, islockHash);
    }
    return GetInstantSendLockByHashInternal(islockHash);
}

std::vector<uint256> CInstantSendDb::GetInstantSendLocksByParent(const uint256& parent) const
{
    AssertLockHeld(cs_db);
    auto it = std::unique_ptr<CDBIterator>(db->NewIterator());
    auto firstKey = std::make_tuple(DB_HASH_BY_OUTPOINT, COutPoint(parent, 0));
    it->Seek(firstKey);

    std::vector<uint256> result;

    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_HASH_BY_OUTPOINT) {
            break;
        }
        const auto& outpoint = std::get<1>(curKey);
        if (outpoint.hash != parent) {
            break;
        }

        uint256 islockHash;
        if (!it->GetValue(islockHash)) {
            break;
        }
        result.emplace_back(islockHash);
        it->Next();
    }

    return result;
}

std::vector<uint256> CInstantSendDb::RemoveChainedInstantSendLocks(const uint256& islockHash, const uint256& txid, int nHeight)
{
    LOCK(cs_db);
    std::vector<uint256> result;

    std::vector<uint256> stack;
    std::unordered_set<uint256, StaticSaltedHasher> added;
    stack.emplace_back(txid);

    CDBBatch batch(*db);
    while (!stack.empty()) {
        auto children = GetInstantSendLocksByParent(stack.back());
        stack.pop_back();

        for (auto& childIslockHash : children) {
            auto childIsLock = GetInstantSendLockByHashInternal(childIslockHash, false);
            if (!childIsLock) {
                continue;
            }

            RemoveInstantSendLock(batch, childIslockHash, childIsLock, false);
            WriteInstantSendLockArchived(batch, childIslockHash, nHeight);
            result.emplace_back(childIslockHash);

            if (added.emplace(childIsLock->txid).second) {
                stack.emplace_back(childIsLock->txid);
            }
        }
    }

    RemoveInstantSendLock(batch, islockHash, nullptr, false);
    WriteInstantSendLockArchived(batch, islockHash, nHeight);
    result.emplace_back(islockHash);

    db->WriteBatch(batch);

    return result;
}

void CInstantSendDb::RemoveAndArchiveInstantSendLock(const gsl::not_null<CInstantSendLockPtr>& islock, int nHeight)
{
    LOCK(cs_db);

    CDBBatch batch(*db);
    const auto hash = ::SerializeHash(*islock);
    RemoveInstantSendLock(batch, hash, islock, false);
    WriteInstantSendLockArchived(batch, hash, nHeight);
    db->WriteBatch(batch);
}

////////////////

std::optional<Consensus::LLMQType> GetInstantSendLLMQTypeAtTip(const CQuorumManager& qman, const CChainState& chainstate)
{
    LOCK(cs_main);
    const CBlockIndex* tip = chainstate.m_chain.Tip();
    if (tip == nullptr) return std::nullopt;
    return std::make_optional(utils::GetInstantSendLLMQType(qman, tip));
}

void CInstantSendManager::Start()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&util::TraceThread, "isman", [this] { WorkThreadMain(); });

    sigman.RegisterRecoveredSigsListener(this);
}

void CInstantSendManager::Stop()
{
    sigman.UnregisterRecoveredSigsListener(this);

    // make sure to call InterruptWorkerThread() first
    if (!workInterrupt) {
        assert(false);
    }

    if (workThread.joinable()) {
        workThread.join();
    }
}

void CInstantSendManager::ProcessTx(const CTransaction& tx, bool fRetroactive, const Consensus::Params& params)
{
    if (!fMasternodeMode || !IsInstantSendEnabled() || !m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (params.llmqTypeInstantSend == Consensus::LLMQType::LLMQ_NONE ||
        params.llmqTypeDIP0024InstantSend == Consensus::LLMQType::LLMQ_NONE) {
        return;
    }

    if (!CheckCanLock(tx, true, params)) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: CheckCanLock returned false\n", __func__,
                  tx.GetHash().ToString());
        return;
    }

    auto conflictingLock = GetConflictingLock(tx);
    if (conflictingLock != nullptr) {
        auto conflictingLockHash = ::SerializeHash(*conflictingLock);
        LogPrintf("CInstantSendManager::%s -- txid=%s: conflicts with islock %s, txid=%s\n", __func__,
                  tx.GetHash().ToString(), conflictingLockHash.ToString(), conflictingLock->txid.ToString());
        return;
    }

    // Only sign for inlocks or islocks if mempool IS signing is enabled.
    // However, if we are processing a tx because it was included in a block we should
    // sign even if mempool IS signing is disabled. This allows a ChainLock to happen on this
    // block after we retroactively locked all transactions.
    if (!IsInstantSendMempoolSigningEnabled() && !fRetroactive) return;

    if (auto llmqType_opt{GetInstantSendLLMQTypeAtTip(qman, m_chainstate)}; !llmqType_opt.has_value()) {
        return;
    } else if (!TrySignInputLocks(tx, fRetroactive, llmqType_opt.value(), params)) {
        return;
    }

    // We might have received all input locks before we got the corresponding TX. In this case, we have to sign the
    // islock now instead of waiting for the input locks.
    TrySignInstantSendLock(tx);
}

bool CInstantSendManager::TrySignInputLocks(const CTransaction& tx, bool fRetroactive, Consensus::LLMQType llmqType, const Consensus::Params& params)
{
    std::vector<uint256> ids;
    ids.reserve(tx.vin.size());

    size_t alreadyVotedCount = 0;
    for (const auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        ids.emplace_back(id);

        uint256 otherTxHash;
        // TODO check that we didn't vote for the other IS type also
        if (sigman.GetVoteForId(params.llmqTypeDIP0024InstantSend, id, otherTxHash) ||
            sigman.GetVoteForId(params.llmqTypeInstantSend, id, otherTxHash)) {
            if (otherTxHash != tx.GetHash()) {
                LogPrintf("CInstantSendManager::%s -- txid=%s: input %s is conflicting with previous vote for tx %s\n", __func__,
                          tx.GetHash().ToString(), in.prevout.ToStringShort(), otherTxHash.ToString());
                return false;
            }
            alreadyVotedCount++;
        }

        // don't even try the actual signing if any input is conflicting
        if (auto llmqs = {params.llmqTypeDIP0024InstantSend, params.llmqTypeInstantSend};
            ranges::any_of(llmqs, [&id, &tx, this](const auto& llmqType){
                return sigman.IsConflicting(llmqType, id, tx.GetHash());})
                ) {
            LogPrintf("CInstantSendManager::%s -- txid=%s: sigman.IsConflicting returned true. id=%s\n", __func__,
                      tx.GetHash().ToString(), id.ToString());
            return false;
        }
    }
    if (!fRetroactive && alreadyVotedCount == ids.size()) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: already voted on all inputs, bailing out\n", __func__,
                 tx.GetHash().ToString());
        return true;
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: trying to vote on %d inputs\n", __func__,
             tx.GetHash().ToString(), tx.vin.size());

    for (const auto i : irange::range(tx.vin.size())) {
        const auto& in = tx.vin[i];
        auto& id = ids[i];
        WITH_LOCK(cs_inputReqests, inputRequestIds.emplace(id));
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: trying to vote on input %s with id %s. fRetroactive=%d\n", __func__,
                 tx.GetHash().ToString(), in.prevout.ToStringShort(), id.ToString(), fRetroactive);
        if (sigman.AsyncSignIfMember(llmqType, shareman, id, tx.GetHash(), {}, fRetroactive)) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: voted on input %s with id %s\n", __func__,
                     tx.GetHash().ToString(), in.prevout.ToStringShort(), id.ToString());
        }
    }

    return true;
}

bool CInstantSendManager::CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params) const
{
    if (tx.vin.empty()) {
        // can't lock TXs without inputs (e.g. quorum commitments)
        return false;
    }

    return ranges::all_of(tx.vin,
                          [&](const auto& in) { return CheckCanLock(in.prevout, printDebug, tx.GetHash(), params); });
}

bool CInstantSendManager::CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256& txHash, const Consensus::Params& params) const
{
    int nInstantSendConfirmationsRequired = params.nInstantSendConfirmationsRequired;

    if (IsLocked(outpoint.hash)) {
        // if prevout was ix locked, allow locking of descendants (no matter if prevout is in mempool or already mined)
        return true;
    }

    auto mempoolTx = mempool.get(outpoint.hash);
    if (mempoolTx) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: parent mempool TX %s is not locked\n", __func__,
                     txHash.ToString(), outpoint.hash.ToString());
        }
        return false;
    }

    uint256 hashBlock;
    CTransactionRef tx = GetTransaction(/* block_index */ nullptr, &mempool, outpoint.hash, params, hashBlock);
    // this relies on enabled txindex and won't work if we ever try to remove the requirement for txindex for masternodes
    if (!tx) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: failed to find parent TX %s\n", __func__,
                     txHash.ToString(), outpoint.hash.ToString());
        }
        return false;
    }

    const CBlockIndex* pindexMined;
    int nTxAge;
    {
        LOCK(cs_main);
        pindexMined = m_chainstate.m_blockman.LookupBlockIndex(hashBlock);
        nTxAge = m_chainstate.m_chain.Height() - pindexMined->nHeight + 1;
    }

    if (nTxAge < nInstantSendConfirmationsRequired && !clhandler.HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: outpoint %s too new and not ChainLocked. nTxAge=%d, nInstantSendConfirmationsRequired=%d\n", __func__,
                     txHash.ToString(), outpoint.ToStringShort(), nTxAge, nInstantSendConfirmationsRequired);
        }
        return false;
    }

    return true;
}

void CInstantSendManager::HandleNewRecoveredSig(const CRecoveredSig& recoveredSig)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    if (Params().GetConsensus().llmqTypeInstantSend == Consensus::LLMQType::LLMQ_NONE ||
        Params().GetConsensus().llmqTypeDIP0024InstantSend == Consensus::LLMQType::LLMQ_NONE) {
        return;
    }

    uint256 txid;
    if (LOCK(cs_inputReqests); inputRequestIds.count(recoveredSig.getId())) {
        txid = recoveredSig.getMsgHash();
    }
    if (!txid.IsNull()) {
        HandleNewInputLockRecoveredSig(recoveredSig, txid);
    } else if (/*isInstantSendLock=*/ WITH_LOCK(cs_creating, return creatingInstantSendLocks.count(recoveredSig.getId()))) {
        HandleNewInstantSendLockRecoveredSig(recoveredSig);
    }
}

void CInstantSendManager::HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid)
{
    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    uint256 hashBlock;
    CTransactionRef tx = GetTransaction(/* block_index */ nullptr, &mempool, txid, Params().GetConsensus(), hashBlock);
    if (!tx) {
        return;
    }

    if (LogAcceptCategory(BCLog::INSTANTSEND)) {
        for (const auto& in : tx->vin) {
            auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
            if (id == recoveredSig.getId()) {
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: got recovered sig for input %s\n", __func__,
                         txid.ToString(), in.prevout.ToStringShort());
                break;
            }
        }
    }

    TrySignInstantSendLock(*tx);
}

void CInstantSendManager::TrySignInstantSendLock(const CTransaction& tx)
{
    const auto llmqType_opt{GetInstantSendLLMQTypeAtTip(qman, m_chainstate)};
    if (!llmqType_opt.has_value()) return;
    const auto llmqType = llmqType_opt.value();

    for (const auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        if (!sigman.HasRecoveredSig(llmqType, id, tx.GetHash())) {
            return;
        }
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: got all recovered sigs, creating CInstantSendLock\n", __func__,
            tx.GetHash().ToString());

    CInstantSendLock islock(llmqType == Params().GetConsensus().llmqTypeDIP0024InstantSend ?
                CInstantSendLock::isdlock_version:
                CInstantSendLock::islock_version);
    islock.txid = tx.GetHash();
    for (const auto& in : tx.vin) {
        islock.inputs.emplace_back(in.prevout);
    }

    // compute and set cycle hash if islock is deterministic
    if (islock.IsDeterministic()) {
        const auto& llmq_params_opt = GetLLMQParams(llmqType);
        assert(llmq_params_opt);
        LOCK(cs_main);
        const auto dkgInterval = llmq_params_opt->dkgInterval;
        const auto quorumHeight = m_chainstate.m_chain.Height() - (m_chainstate.m_chain.Height() % dkgInterval);
        islock.cycleHash = m_chainstate.m_chain[quorumHeight]->GetBlockHash();
    }

    auto id = islock.GetRequestId();

    if (sigman.HasRecoveredSigForId(llmqType, id)) {
        return;
    }

    {
        LOCK(cs_creating);
        auto e = creatingInstantSendLocks.emplace(id, std::move(islock));
        if (!e.second) {
            return;
        }
        txToCreatingInstantSendLocks.emplace(tx.GetHash(), &e.first->second);
    }

    sigman.AsyncSignIfMember(llmqType, shareman, id, tx.GetHash());
}

void CInstantSendManager::HandleNewInstantSendLockRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    CInstantSendLockPtr islock;

    {
        LOCK(cs_creating);
        auto it = creatingInstantSendLocks.find(recoveredSig.getId());
        if (it == creatingInstantSendLocks.end()) {
            return;
        }

        islock = std::make_shared<CInstantSendLock>(std::move(it->second));
        creatingInstantSendLocks.erase(it);
        txToCreatingInstantSendLocks.erase(islock->txid);
    }

    if (islock->txid != recoveredSig.getMsgHash()) {
        LogPrintf("CInstantSendManager::%s -- txid=%s: islock conflicts with %s, dropping own version\n", __func__,
                islock->txid.ToString(), recoveredSig.getMsgHash().ToString());
        return;
    }

    islock->sig = recoveredSig.sig;
    auto hash = ::SerializeHash(*islock);

    if (WITH_LOCK(cs_pendingLocks, return pendingInstantSendLocks.count(hash)) || db.KnownInstantSendLock(hash)) {
        return;
    }
    LOCK(cs_pendingLocks);
    pendingInstantSendLocks.emplace(hash, std::make_pair(-1, islock));
}

void CInstantSendManager::ProcessMessage(const CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    if (msg_type == NetMsgType::ISLOCK || msg_type == NetMsgType::ISDLOCK) {
        const auto islock_version = msg_type == NetMsgType::ISLOCK ? CInstantSendLock::islock_version : CInstantSendLock::isdlock_version;
        const auto islock = std::make_shared<CInstantSendLock>(islock_version);
        vRecv >> *islock;
        ProcessMessageInstantSendLock(pfrom, islock);
    }
}

void CInstantSendManager::ProcessMessageInstantSendLock(const CNode& pfrom, const llmq::CInstantSendLockPtr& islock)
{
    auto hash = ::SerializeHash(*islock);

    bool fDIP0024IsActive = false;
    {
        LOCK(cs_main);
        EraseObjectRequest(pfrom.GetId(), CInv(islock->IsDeterministic() ? MSG_ISDLOCK : MSG_ISLOCK, hash));
        fDIP0024IsActive = utils::IsDIP0024Active(m_chainstate.m_chain.Tip());
    }

    if (!islock->TriviallyValid()) {
        m_peerman->Misbehaving(pfrom.GetId(), 100);
        return;
    }

    // Deterministic ISLocks are only produced by rotation quorums, if we don't see DIP24 as active, then we won't be able to validate it anyway
    if (islock->IsDeterministic() && fDIP0024IsActive) {
        const auto blockIndex = WITH_LOCK(cs_main, return m_chainstate.m_blockman.LookupBlockIndex(islock->cycleHash));
        if (blockIndex == nullptr) {
            // Maybe we don't have the block yet or maybe some peer spams invalid values for cycleHash
            m_peerman->Misbehaving(pfrom.GetId(), 1);
            return;
        }

        // Deterministic islocks MUST use rotation based llmq
        auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;
        const auto& llmq_params_opt = GetLLMQParams(llmqType);
        assert(llmq_params_opt);
        if (blockIndex->nHeight % llmq_params_opt->dkgInterval != 0) {
            m_peerman->Misbehaving(pfrom.GetId(), 100);
            return;
        }
    }

    // WE MUST STILL PROCESS OLD ISLOCKS?
//    else if (utils::IsDIP0024Active(WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip()))) {
//        // Ignore non-deterministic islocks once rotation is active
//        return;
//    }

    if (WITH_LOCK(cs_pendingLocks, return pendingInstantSendLocks.count(hash) || pendingNoTxInstantSendLocks.count(hash))
            || db.KnownInstantSendLock(hash)) {
        return;
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: received islock, peer=%d\n", __func__,
            islock->txid.ToString(), hash.ToString(), pfrom.GetId());

    LOCK(cs_pendingLocks);
    pendingInstantSendLocks.emplace(hash, std::make_pair(pfrom.GetId(), islock));
}

/**
 * Handles trivial ISLock verification
 * @return returns false if verification failed, otherwise true
 */
bool CInstantSendLock::TriviallyValid() const
{
    if (txid.IsNull() || inputs.empty()) {
        return false;
    }

    // Check that each input is unique
    std::set<COutPoint> dups;
    for (const auto& o : inputs) {
        if (!dups.emplace(o).second) {
            return false;
        }
    }

    return true;
}

bool CInstantSendManager::ProcessPendingInstantSendLocks()
{
    if (auto llmqType_opt{GetInstantSendLLMQTypeAtTip(qman, m_chainstate)}; !llmqType_opt.has_value()) {
        return true; // not an error
    } else if (llmqType_opt.value() == Params().GetConsensus().llmqTypeDIP0024InstantSend) {
         // Don't short circuit. Try to process both deterministic and not deterministic islocks independable
        bool deterministicRes = ProcessPendingInstantSendLocks(true);
        bool nondeterministicRes = ProcessPendingInstantSendLocks(false);
        return deterministicRes && nondeterministicRes;
    } else {
        return ProcessPendingInstantSendLocks(false);
    }
}

bool CInstantSendManager::ProcessPendingInstantSendLocks(bool deterministic)
{
    decltype(pendingInstantSendLocks) pend;
    bool fMoreWork{false};

    if (!IsInstantSendEnabled()) {
        return false;
    }

    {
        LOCK(cs_pendingLocks);
        // only process a max 32 locks at a time to avoid duplicate verification of recovered signatures which have been
        // verified by CSigningManager in parallel
        const size_t maxCount = 32;
        // The keys of the removed values are temporaily stored here to avoid invalidating an iterator
        std::vector<uint256> removed;
        removed.reserve(maxCount);

        for (const auto& [islockHash, nodeid_islptr_pair] : pendingInstantSendLocks) {
            const auto& [_, islock_ptr] = nodeid_islptr_pair;
            // Check if we've reached max count
            if (pend.size() >= maxCount) {
                fMoreWork = true;
                break;
            }
            // Check if we care about this islock on this run
            if (islock_ptr->IsDeterministic() != deterministic) {
                continue;
            }
            pend.emplace(islockHash, std::move(nodeid_islptr_pair));
            removed.emplace_back(islockHash);
        }

        for (const auto& islockHash : removed) {
            pendingInstantSendLocks.erase(islockHash);
        }
    }

    if (pend.empty()) {
        return false;
    }

    //TODO Investigate if leaving this is ok
    auto llmqType = utils::GetInstantSendLLMQType(deterministic);
    const auto& llmq_params_opt = GetLLMQParams(llmqType);
    assert(llmq_params_opt);
    const auto& llmq_params = llmq_params_opt.value();
    auto dkgInterval = llmq_params.dkgInterval;

    // First check against the current active set and don't ban
    auto badISLocks = ProcessPendingInstantSendLocks(llmq_params, 0, pend, false);
    if (!badISLocks.empty()) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- doing verification on old active set\n", __func__);

        // filter out valid IS locks from "pend"
        for (auto it = pend.begin(); it != pend.end(); ) {
            if (!badISLocks.count(it->first)) {
                it = pend.erase(it);
            } else {
                ++it;
            }
        }
        // Now check against the previous active set and perform banning if this fails
        ProcessPendingInstantSendLocks(llmq_params, dkgInterval, pend, true);
    }

    return fMoreWork;
}

std::unordered_set<uint256, StaticSaltedHasher> CInstantSendManager::ProcessPendingInstantSendLocks(const Consensus::LLMQParams& llmq_params, int signOffset, const std::unordered_map<uint256, std::pair<NodeId, CInstantSendLockPtr>, StaticSaltedHasher>& pend, bool ban)
{
    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, true, 8);
    std::unordered_map<uint256, CRecoveredSig, StaticSaltedHasher> recSigs;

    size_t verifyCount = 0;
    size_t alreadyVerified = 0;
    for (const auto& p : pend) {
        const auto& hash = p.first;
        auto nodeId = p.second.first;
        const auto& islock = p.second.second;

        if (batchVerifier.badSources.count(nodeId)) {
            continue;
        }

        if (!islock->sig.Get().IsValid()) {
            batchVerifier.badSources.emplace(nodeId);
            continue;
        }

        auto id = islock->GetRequestId();

        // no need to verify an ISLOCK if we already have verified the recovered sig that belongs to it
        if (sigman.HasRecoveredSig(llmq_params.type, id, islock->txid)) {
            alreadyVerified++;
            continue;
        }

        int nSignHeight{-1};
        if (islock->IsDeterministic()) {
            LOCK(cs_main);

            const auto blockIndex = m_chainstate.m_blockman.LookupBlockIndex(islock->cycleHash);
            if (blockIndex == nullptr) {
                batchVerifier.badSources.emplace(nodeId);
                continue;
            }

            const auto dkgInterval = llmq_params.dkgInterval;
            if (blockIndex->nHeight + dkgInterval < m_chainstate.m_chain.Height()) {
                nSignHeight = blockIndex->nHeight + dkgInterval - 1;
            }
        }

        auto quorum = llmq::CSigningManager::SelectQuorumForSigning(llmq_params, qman, id, nSignHeight, signOffset);
        if (!quorum) {
            // should not happen, but if one fails to select, all others will also fail to select
            return {};
        }
        uint256 signHash = utils::BuildSignHash(llmq_params.type, quorum->qc->quorumHash, id, islock->txid);
        batchVerifier.PushMessage(nodeId, hash, signHash, islock->sig.Get(), quorum->qc->quorumPublicKey);
        verifyCount++;

        // We can reconstruct the CRecoveredSig objects from the islock and pass it to the signing manager, which
        // avoids unnecessary double-verification of the signature. We however only do this when verification here
        // turns out to be good (which is checked further down)
        if (!sigman.HasRecoveredSigForId(llmq_params.type, id)) {
            recSigs.try_emplace(hash, CRecoveredSig(llmq_params.type, quorum->qc->quorumHash, id, islock->txid, islock->sig));
        }
    }

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- verified locks. count=%d, alreadyVerified=%d, vt=%d, nodes=%d\n", __func__,
            verifyCount, alreadyVerified, verifyTimer.count(), batchVerifier.GetUniqueSourceCount());

    std::unordered_set<uint256, StaticSaltedHasher> badISLocks;

    if (ban && !batchVerifier.badSources.empty()) {
        LOCK(cs_main);
        for (const auto& nodeId : batchVerifier.badSources) {
            // Let's not be too harsh, as the peer might simply be unlucky and might have sent us an old lock which
            // does not validate anymore due to changed quorums
            m_peerman->Misbehaving(nodeId, 20);
        }
    }
    for (const auto& p : pend) {
        const auto& hash = p.first;
        auto nodeId = p.second.first;
        const auto& islock = p.second.second;

        if (batchVerifier.badMessages.count(hash)) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: invalid sig in islock, peer=%d\n", __func__,
                     islock->txid.ToString(), hash.ToString(), nodeId);
            badISLocks.emplace(hash);
            continue;
        }

        ProcessInstantSendLock(nodeId, hash, islock);

        // See comment further on top. We pass a reconstructed recovered sig to the signing manager to avoid
        // double-verification of the sig.
        auto it = recSigs.find(hash);
        if (it != recSigs.end()) {
            auto recSig = std::make_shared<CRecoveredSig>(std::move(it->second));
            if (!sigman.HasRecoveredSigForId(llmq_params.type, recSig->getId())) {
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: passing reconstructed recSig to signing mgr, peer=%d\n", __func__,
                         islock->txid.ToString(), hash.ToString(), nodeId);
                sigman.PushReconstructedRecoveredSig(recSig);
            }
        }
    }

    return badISLocks;
}

void CInstantSendManager::ProcessInstantSendLock(NodeId from, const uint256& hash, const CInstantSendLockPtr& islock)
{
    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: processing islock, peer=%d\n", __func__,
             islock->txid.ToString(), hash.ToString(), from);
    {
        LOCK(cs_creating);
        creatingInstantSendLocks.erase(islock->GetRequestId());
        txToCreatingInstantSendLocks.erase(islock->txid);
    }
    if (db.KnownInstantSendLock(hash)) {
        return;
    }

    uint256 hashBlock;
    CTransactionRef tx = GetTransaction(/* block_index */ nullptr, &mempool, islock->txid, Params().GetConsensus(), hashBlock);
    const CBlockIndex* pindexMined{nullptr};
    // we ignore failure here as we must be able to propagate the lock even if we don't have the TX locally
    if (tx && !hashBlock.IsNull()) {
        pindexMined = WITH_LOCK(cs_main, return m_chainstate.m_blockman.LookupBlockIndex(hashBlock));

        // Let's see if the TX that was locked by this islock is already mined in a ChainLocked block. If yes,
        // we can simply ignore the islock, as the ChainLock implies locking of all TXs in that chain
        if (pindexMined != nullptr && clhandler.HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txlock=%s, islock=%s: dropping islock as it already got a ChainLock in block %s, peer=%d\n", __func__,
                     islock->txid.ToString(), hash.ToString(), hashBlock.ToString(), from);
            return;
        }
    }

    const auto sameTxIsLock = db.GetInstantSendLockByTxid(islock->txid);
    if (sameTxIsLock != nullptr) {
        if (sameTxIsLock->IsDeterministic() == islock->IsDeterministic()) {
            // shouldn't happen, investigate
            LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: duplicate islock, other islock=%s, peer=%d\n", __func__,
                      islock->txid.ToString(), hash.ToString(), ::SerializeHash(*sameTxIsLock).ToString(), from);
        }
        if (sameTxIsLock->IsDeterministic()) {
            // can happen, nothing to do
            return;
        } else if (islock->IsDeterministic()) {
            // can happen, remove and archive the non-deterministic sameTxIsLock
            db.RemoveAndArchiveInstantSendLock(sameTxIsLock, WITH_LOCK(::cs_main, return m_chainstate.m_chain.Height()));
        }
    } else {
        for (const auto& in : islock->inputs) {
            const auto sameOutpointIsLock = db.GetInstantSendLockByInput(in);
            if (sameOutpointIsLock != nullptr) {
                LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: conflicting outpoint in islock. input=%s, other islock=%s, peer=%d\n", __func__,
                          islock->txid.ToString(), hash.ToString(), in.ToStringShort(), ::SerializeHash(*sameOutpointIsLock).ToString(), from);
            }
        }
    }

    if (tx == nullptr) {
        // put it in a separate pending map and try again later
        LOCK(cs_pendingLocks);
        pendingNoTxInstantSendLocks.try_emplace(hash, std::make_pair(from, islock));
    } else {
        db.WriteNewInstantSendLock(hash, *islock);
        if (pindexMined) {
            db.WriteInstantSendLockMined(hash, pindexMined->nHeight);
        }
    }

    // This will also add children TXs to pendingRetryTxs
    RemoveNonLockedTx(islock->txid, true);
    // We don't need the recovered sigs for the inputs anymore. This prevents unnecessary propagation of these sigs.
    // We only need the ISLOCK from now on to detect conflicts
    TruncateRecoveredSigsForInputs(*islock);

    const auto is_det = islock->IsDeterministic();
    CInv inv(is_det ? MSG_ISDLOCK : MSG_ISLOCK, hash);
    if (tx != nullptr) {
        connman.RelayInvFiltered(inv, *tx, is_det ? ISDLOCK_PROTO_VERSION : MIN_PEER_PROTO_VERSION);
    } else {
        // we don't have the TX yet, so we only filter based on txid. Later when that TX arrives, we will re-announce
        // with the TX taken into account.
        connman.RelayInvFiltered(inv, islock->txid, is_det ? ISDLOCK_PROTO_VERSION : MIN_PEER_PROTO_VERSION);
    }

    ResolveBlockConflicts(hash, *islock);

    if (tx != nullptr) {
        RemoveMempoolConflictsForLock(hash, *islock);
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- notify about lock %s for tx %s\n", __func__,
                hash.ToString(), tx->GetHash().ToString());
        GetMainSignals().NotifyTransactionLock(tx, islock);
        // bump mempool counter to make sure newly locked txes are picked up by getblocktemplate
        mempool.AddTransactionsUpdated(1);
    } else {
        AskNodesForLockedTx(islock->txid, connman);
    }
}

void CInstantSendManager::TransactionAddedToMempool(const CTransactionRef& tx)
{
    if (!IsInstantSendEnabled() || !m_mn_sync.IsBlockchainSynced() || tx->vin.empty()) {
        return;
    }

    CInstantSendLockPtr islock{nullptr};
    {
        LOCK(cs_pendingLocks);
        auto it = pendingNoTxInstantSendLocks.begin();
        while (it != pendingNoTxInstantSendLocks.end()) {
            if (it->second.second->txid == tx->GetHash()) {
                // we received an islock earlier
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__,
                         tx->GetHash().ToString(), it->first.ToString());
                islock = it->second.second;
                pendingInstantSendLocks.try_emplace(it->first, it->second);
                pendingNoTxInstantSendLocks.erase(it);
                break;
            }
            ++it;
        }
    }

    if (islock == nullptr) {
        ProcessTx(*tx, false, Params().GetConsensus());
        // TX is not locked, so make sure it is tracked
        AddNonLockedTx(tx, nullptr);
    } else {
        RemoveMempoolConflictsForLock(::SerializeHash(*islock), *islock);
    }
}

void CInstantSendManager::TransactionRemovedFromMempool(const CTransactionRef& tx)
{
    if (tx->vin.empty() || !fUpgradedDB) {
        return;
    }

    CInstantSendLockPtr islock = db.GetInstantSendLockByTxid(tx->GetHash());

    if (islock == nullptr) {
        return;
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- transaction %s was removed from mempool\n", __func__, tx->GetHash().ToString());
    RemoveConflictingLock(::SerializeHash(*islock), *islock);
}

void CInstantSendManager::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    if (m_mn_sync.IsBlockchainSynced()) {
        for (const auto& tx : pblock->vtx) {
            if (tx->IsCoinBase() || tx->vin.empty()) {
                // coinbase and TXs with no inputs can't be locked
                continue;
            }

            if (!IsLocked(tx->GetHash()) && !clhandler.HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
                ProcessTx(*tx, true, Params().GetConsensus());
                // TX is not locked, so make sure it is tracked
                AddNonLockedTx(tx, pindex);
            } else {
                // TX is locked, so make sure we don't track it anymore
                RemoveNonLockedTx(tx->GetHash(), true);
            }
        }
    }

    db.WriteBlockInstantSendLocks(pblock, pindex);
}

void CInstantSendManager::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    db.RemoveBlockInstantSendLocks(pblock, pindexDisconnected);
}

void CInstantSendManager::AddNonLockedTx(const CTransactionRef& tx, const CBlockIndex* pindexMined)
{
    {
        LOCK(cs_nonLocked);
        auto [it, did_insert] = nonLockedTxs.emplace(tx->GetHash(), NonLockedTxInfo());
        auto& nonLockedTxInfo = it->second;
        nonLockedTxInfo.pindexMined = pindexMined;

        if (did_insert) {
            nonLockedTxInfo.tx = tx;
            for (const auto &in: tx->vin) {
                nonLockedTxs[in.prevout.hash].children.emplace(tx->GetHash());
                nonLockedTxsByOutpoints.emplace(in.prevout, tx->GetHash());
            }
        }
    }
    {
        LOCK(cs_pendingLocks);
        auto it = pendingNoTxInstantSendLocks.begin();
        while (it != pendingNoTxInstantSendLocks.end()) {
            if (it->second.second->txid == tx->GetHash()) {
                // we received an islock earlier, let's put it back into pending and verify/lock
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__,
                         tx->GetHash().ToString(), it->first.ToString());
                pendingInstantSendLocks.try_emplace(it->first, it->second);
                pendingNoTxInstantSendLocks.erase(it);
                break;
            }
            ++it;
        }
    }
    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, pindexMined=%s\n", __func__,
             tx->GetHash().ToString(), pindexMined ? pindexMined->GetBlockHash().ToString() : "");
}

void CInstantSendManager::RemoveNonLockedTx(const uint256& txid, bool retryChildren)
{
    LOCK(cs_nonLocked);

    auto it = nonLockedTxs.find(txid);
    if (it == nonLockedTxs.end()) {
        return;
    }
    const auto& info = it->second;

    size_t retryChildrenCount = 0;
    if (retryChildren) {
        // TX got locked, so we can retry locking children
        LOCK(cs_pendingRetry);
        for (const auto& childTxid : info.children) {
            pendingRetryTxs.emplace(childTxid);
            retryChildrenCount++;
        }
    }

    if (info.tx) {
        for (const auto& in : info.tx->vin) {
            auto jt = nonLockedTxs.find(in.prevout.hash);
            if (jt != nonLockedTxs.end()) {
                jt->second.children.erase(txid);
                if (!jt->second.tx && jt->second.children.empty()) {
                    nonLockedTxs.erase(jt);
                }
            }
            nonLockedTxsByOutpoints.erase(in.prevout);
        }
    }

    nonLockedTxs.erase(it);

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, retryChildren=%d, retryChildrenCount=%d\n", __func__,
             txid.ToString(), retryChildren, retryChildrenCount);
}

void CInstantSendManager::RemoveConflictedTx(const CTransaction& tx)
{
    RemoveNonLockedTx(tx.GetHash(), false);

    LOCK(cs_inputReqests);
    for (const auto& in : tx.vin) {
        auto inputRequestId = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in));
        inputRequestIds.erase(inputRequestId);
    }
}

void CInstantSendManager::TruncateRecoveredSigsForInputs(const llmq::CInstantSendLock& islock)
{
    for (const auto& in : islock.inputs) {
        auto inputRequestId = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in));
        WITH_LOCK(cs_inputReqests, inputRequestIds.erase(inputRequestId));
        sigman.TruncateRecoveredSig(utils::GetInstantSendLLMQType(islock.IsDeterministic()), inputRequestId);
    }
}

void CInstantSendManager::NotifyChainLock(const CBlockIndex* pindexChainLock)
{
    HandleFullyConfirmedBlock(pindexChainLock);
}

void CInstantSendManager::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    if (!fUpgradedDB) {
        if (pindexNew->nHeight + 1 >= Params().GetConsensus().DIP0020Height) {
            db.Upgrade(mempool);
            fUpgradedDB = true;
        }
    }

    bool fDIP0008Active = pindexNew->pprev && pindexNew->pprev->nHeight >= Params().GetConsensus().DIP0008Height;

    if (AreChainLocksEnabled(spork_manager) && fDIP0008Active) {
        // Nothing to do here. We should keep all islocks and let chainlocks handle them.
        return;
    }

    int nConfirmedHeight = pindexNew->nHeight - Params().GetConsensus().nInstantSendKeepLock;
    const CBlockIndex* pindex = pindexNew->GetAncestor(nConfirmedHeight);

    if (pindex) {
        HandleFullyConfirmedBlock(pindex);
    }
}

void CInstantSendManager::HandleFullyConfirmedBlock(const CBlockIndex* pindex)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    auto removeISLocks = db.RemoveConfirmedInstantSendLocks(pindex->nHeight);

    for (const auto& [islockHash, islock] : removeISLocks) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: removed islock as it got fully confirmed\n", __func__,
                 islock->txid.ToString(), islockHash.ToString());

        // No need to keep recovered sigs for fully confirmed IS locks, as there is no chance for conflicts
        // from now on. All inputs are spent now and can't be spend in any other TX.
        TruncateRecoveredSigsForInputs(*islock);

        // And we don't need the recovered sig for the ISLOCK anymore, as the block in which it got mined is considered
        // fully confirmed now
        sigman.TruncateRecoveredSig(utils::GetInstantSendLLMQType(islock->IsDeterministic()), islock->GetRequestId());
    }

    db.RemoveArchivedInstantSendLocks(pindex->nHeight - 100);

    // Find all previously unlocked TXs that got locked by this fully confirmed (ChainLock) block and remove them
    // from the nonLockedTxs map. Also collect all children of these TXs and mark them for retrying of IS locking.
    std::vector<uint256> toRemove;
    {
        LOCK(cs_nonLocked);
        for (const auto& p: nonLockedTxs) {
            const auto* pindexMined = p.second.pindexMined;

            if (pindexMined && pindex->GetAncestor(pindexMined->nHeight) == pindexMined) {
                toRemove.emplace_back(p.first);
            }
        }
    }
    for (const auto& txid : toRemove) {
        // This will also add children to pendingRetryTxs
        RemoveNonLockedTx(txid, true);
    }
}

void CInstantSendManager::RemoveMempoolConflictsForLock(const uint256& hash, const CInstantSendLock& islock)
{
    std::unordered_map<uint256, CTransactionRef, StaticSaltedHasher> toDelete;

    {
        LOCK(mempool.cs);

        for (const auto& in : islock.inputs) {
            auto it = mempool.mapNextTx.find(in);
            if (it == mempool.mapNextTx.end()) {
                continue;
            }
            if (it->second->GetHash() != islock.txid) {
                toDelete.emplace(it->second->GetHash(), mempool.get(it->second->GetHash()));

                LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: mempool TX %s with input %s conflicts with islock\n", __func__,
                         islock.txid.ToString(), hash.ToString(), it->second->GetHash().ToString(), in.ToStringShort());
            }
        }

        for (const auto& p : toDelete) {
            mempool.removeRecursive(*p.second, MemPoolRemovalReason::CONFLICT);
        }
    }

    if (!toDelete.empty()) {
        for (const auto& p : toDelete) {
            RemoveConflictedTx(*p.second);
        }
        AskNodesForLockedTx(islock.txid, connman);
    }
}

void CInstantSendManager::ResolveBlockConflicts(const uint256& islockHash, const llmq::CInstantSendLock& islock)
{
    // Lets first collect all non-locked TXs which conflict with the given ISLOCK
    std::unordered_map<const CBlockIndex*, std::unordered_map<uint256, CTransactionRef, StaticSaltedHasher>> conflicts;
    {
        LOCK(cs_nonLocked);
        for (const auto& in : islock.inputs) {
            auto it = nonLockedTxsByOutpoints.find(in);
            if (it != nonLockedTxsByOutpoints.end()) {
                auto& conflictTxid = it->second;
                if (conflictTxid == islock.txid) {
                    continue;
                }
                auto jt = nonLockedTxs.find(conflictTxid);
                if (jt == nonLockedTxs.end()) {
                    continue;
                }
                auto& info = jt->second;
                if (!info.pindexMined || !info.tx) {
                    continue;
                }
                LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: mined TX %s with input %s and mined in block %s conflicts with islock\n", __func__,
                          islock.txid.ToString(), islockHash.ToString(), conflictTxid.ToString(), in.ToStringShort(), info.pindexMined->GetBlockHash().ToString());
                conflicts[info.pindexMined].emplace(conflictTxid, info.tx);
            }
        }
    }

    // Lets see if any of the conflicts was already mined into a ChainLocked block
    bool hasChainLockedConflict = false;
    for (const auto& p : conflicts) {
        const auto* pindex = p.first;
        if (clhandler.HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            hasChainLockedConflict = true;
            break;
        }
    }

    // If a conflict was mined into a ChainLocked block, then we have no other choice and must prune the ISLOCK and all
    // chained ISLOCKs that build on top of this one. The probability of this is practically zero and can only happen
    // when large parts of the masternode network are controlled by an attacker. In this case we must still find consensus
    // and its better to sacrifice individual ISLOCKs then to sacrifice whole ChainLocks.
    if (hasChainLockedConflict) {
        LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: at least one conflicted TX already got a ChainLock\n", __func__,
                  islock.txid.ToString(), islockHash.ToString());
        RemoveConflictingLock(islockHash, islock);
        return;
    }

    bool isLockedTxKnown = WITH_LOCK(cs_pendingLocks, return pendingNoTxInstantSendLocks.find(islockHash) == pendingNoTxInstantSendLocks.end());

    bool activateBestChain = false;
    for (const auto& p : conflicts) {
        const auto* pindex = p.first;
        for (const auto& p2 : p.second) {
            const auto& tx = *p2.second;
            RemoveConflictedTx(tx);
        }

        LogPrintf("CInstantSendManager::%s -- invalidating block %s\n", __func__, pindex->GetBlockHash().ToString());

        BlockValidationState state;
        // need non-const pointer
        auto pindex2 = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(pindex->GetBlockHash()));
        if (!m_chainstate.InvalidateBlock(state, pindex2)) {
            LogPrintf("CInstantSendManager::%s -- InvalidateBlock failed: %s\n", __func__, state.ToString());
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
        if (isLockedTxKnown) {
            activateBestChain = true;
        } else {
            LogPrintf("CInstantSendManager::%s -- resetting block %s\n", __func__, pindex2->GetBlockHash().ToString());
            LOCK(cs_main);
            m_chainstate.ResetBlockFailureFlags(pindex2);
        }
    }

    if (activateBestChain) {
        BlockValidationState state;
        if (!m_chainstate.ActivateBestChain(state)) {
            LogPrintf("CChainLocksHandler::%s -- ActivateBestChain failed: %s\n", __func__, state.ToString());
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
    }
}

void CInstantSendManager::RemoveConflictingLock(const uint256& islockHash, const llmq::CInstantSendLock& islock)
{
    LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: Removing ISLOCK and its chained children\n", __func__,
              islock.txid.ToString(), islockHash.ToString());
    int tipHeight = WITH_LOCK(cs_main, return m_chainstate.m_chain.Height());

    auto removedIslocks = db.RemoveChainedInstantSendLocks(islockHash, islock.txid, tipHeight);
    for (const auto& h : removedIslocks) {
        LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: removed (child) ISLOCK %s\n", __func__,
                  islock.txid.ToString(), islockHash.ToString(), h.ToString());
    }
}

void CInstantSendManager::AskNodesForLockedTx(const uint256& txid, const CConnman& connman)
{
    std::vector<CNode*> nodesToAskFor;
    nodesToAskFor.reserve(4);

    auto maybe_add_to_nodesToAskFor = [&nodesToAskFor, &txid](CNode* pnode) {
        if (nodesToAskFor.size() >= 4) {
            return;
        }
        if (pnode->IsAddrRelayPeer()) {
            LOCK(pnode->m_tx_relay->cs_tx_inventory);
            if (pnode->m_tx_relay->filterInventoryKnown.contains(txid)) {
                pnode->AddRef();
                nodesToAskFor.emplace_back(pnode);
            }
        }
    };

    connman.ForEachNode([&](CNode* pnode) {
        // Check masternodes first
        if (pnode->m_masternode_connection) maybe_add_to_nodesToAskFor(pnode);
    });
    connman.ForEachNode([&](CNode* pnode) {
        // Check non-masternodes next
        if (!pnode->m_masternode_connection) maybe_add_to_nodesToAskFor(pnode);
    });
    {
        LOCK(cs_main);
        for (const CNode* pnode : nodesToAskFor) {
            LogPrintf("CInstantSendManager::%s -- txid=%s: asking other peer %d for correct TX\n", __func__,
                      txid.ToString(), pnode->GetId());

            CInv inv(MSG_TX, txid);
            RequestObject(pnode->GetId(), inv, GetTime<std::chrono::microseconds>(), true);
        }
    }
    for (CNode* pnode : nodesToAskFor) {
        pnode->Release();
    }
}

void CInstantSendManager::ProcessPendingRetryLockTxs()
{
    decltype(pendingRetryTxs) retryTxs = WITH_LOCK(cs_pendingRetry, return std::move(pendingRetryTxs));

    if (retryTxs.empty()) {
        return;
    }

    if (!IsInstantSendEnabled()) {
        return;
    }

    int retryCount = 0;
    for (const auto& txid : retryTxs) {
        CTransactionRef tx;
        {
            {
                LOCK(cs_nonLocked);
                auto it = nonLockedTxs.find(txid);
                if (it == nonLockedTxs.end()) {
                    continue;
                }
                tx = it->second.tx;
            }
            if (!tx) {
                continue;
            }

            if (LOCK(cs_creating); txToCreatingInstantSendLocks.count(tx->GetHash())) {
                // we're already in the middle of locking this one
                continue;
            }
            if (IsLocked(tx->GetHash())) {
                continue;
            }
            if (GetConflictingLock(*tx) != nullptr) {
                // should not really happen as we have already filtered these out
                continue;
            }
        }

        // CheckCanLock is already called by ProcessTx, so we should avoid calling it twice. But we also shouldn't spam
        // the logs when retrying TXs that are not ready yet.
        if (LogAcceptCategory(BCLog::INSTANTSEND)) {
            if (!CheckCanLock(*tx, false, Params().GetConsensus())) {
                continue;
            }
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: retrying to lock\n", __func__,
                     tx->GetHash().ToString());
        }

        ProcessTx(*tx, false, Params().GetConsensus());
        retryCount++;
    }

    if (retryCount != 0) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- retried %d TXs. nonLockedTxs.size=%d\n", __func__,
                 retryCount, WITH_LOCK(cs_nonLocked, return nonLockedTxs.size()));
    }
}

bool CInstantSendManager::AlreadyHave(const CInv& inv) const
{
    if (!IsInstantSendEnabled()) {
        return true;
    }

    return WITH_LOCK(cs_pendingLocks, return pendingInstantSendLocks.count(inv.hash) != 0 || pendingNoTxInstantSendLocks.count(inv.hash) != 0)
            || db.KnownInstantSendLock(inv.hash);
}

bool CInstantSendManager::GetInstantSendLockByHash(const uint256& hash, llmq::CInstantSendLock& ret) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    auto islock = db.GetInstantSendLockByHash(hash);
    if (!islock) {
        LOCK(cs_pendingLocks);
        auto it = pendingInstantSendLocks.find(hash);
        if (it != pendingInstantSendLocks.end()) {
            islock = it->second.second;
        } else {
            auto itNoTx = pendingNoTxInstantSendLocks.find(hash);
            if (itNoTx != pendingNoTxInstantSendLocks.end()) {
                islock = itNoTx->second.second;
            } else {
                return false;
            }
        }
    }
    ret = *islock;
    return true;
}

CInstantSendLockPtr CInstantSendManager::GetInstantSendLockByTxid(const uint256& txid) const
{
    if (!IsInstantSendEnabled()) {
        return nullptr;
    }

    return db.GetInstantSendLockByTxid(txid);
}

bool CInstantSendManager::IsLocked(const uint256& txHash) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    return db.KnownInstantSendLock(db.GetInstantSendLockHashByTxid(txHash));
}

bool CInstantSendManager::IsWaitingForTx(const uint256& txHash) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    LOCK(cs_pendingLocks);
    auto it = pendingNoTxInstantSendLocks.begin();
    while (it != pendingNoTxInstantSendLocks.end()) {
        if (it->second.second->txid == txHash) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__,
                     txHash.ToString(), it->first.ToString());
            return true;
        }
        ++it;
    }
    return false;
}

CInstantSendLockPtr CInstantSendManager::GetConflictingLock(const CTransaction& tx) const
{
    if (!IsInstantSendEnabled()) {
        return nullptr;
    }

    for (const auto& in : tx.vin) {
        auto otherIsLock = db.GetInstantSendLockByInput(in.prevout);
        if (!otherIsLock) {
            continue;
        }

        if (otherIsLock->txid != tx.GetHash()) {
            return otherIsLock;
        }
    }
    return nullptr;
}

size_t CInstantSendManager::GetInstantSendLockCount() const
{
    return db.GetInstantSendLockCount();
}

void CInstantSendManager::WorkThreadMain()
{
    while (!workInterrupt) {
        bool fMoreWork = ProcessPendingInstantSendLocks();
        ProcessPendingRetryLockTxs();

        if (!fMoreWork && !workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
            return;
        }
    }
}

bool CInstantSendManager::IsInstantSendEnabled() const
{
    return !fReindex && !fImporting && spork_manager.IsSporkActive(SPORK_2_INSTANTSEND_ENABLED);
}

bool CInstantSendManager::IsInstantSendMempoolSigningEnabled() const
{
    return !fReindex && !fImporting && spork_manager.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED) == 0;
}

bool CInstantSendManager::RejectConflictingBlocks() const
{
    if (!m_mn_sync.IsBlockchainSynced()) {
        return false;
    }
    if (!spork_manager.IsSporkActive(SPORK_3_INSTANTSEND_BLOCK_FILTERING)) {
        LogPrint(BCLog::INSTANTSEND, "%s: spork3 is off, skipping transaction locking checks\n", __func__);
        return false;
    }
    return true;
}

} // namespace llmq
