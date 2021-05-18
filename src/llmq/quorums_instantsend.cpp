// Copyright (c) 2019-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_utils.h>

#include <bls/bls_batchverifier.h>
#include <chainparams.h>
#include <txmempool.h>
#include <masternode/masternode-sync.h>
#include <net_processing.h>
#include <spork.h>
#include <validation.h>

#include <cxxtimer.hpp>

#include <boost/algorithm/string/replace.hpp>

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

CInstantSendManager* quorumInstantSendManager;

uint256 CInstantSendLock::GetRequestId() const
{
    CHashWriter hw(SER_GETHASH, 0);
    hw << ISLOCK_REQUESTID_PREFIX;
    hw << inputs;
    return hw.GetHash();
}

////////////////

CInstantSendDb::CInstantSendDb(CDBWrapper& _db) : db(_db)
{
}

void CInstantSendDb::Upgrade()
{
    int v{0};
    if (!db.Read(DB_VERSION, v) || v < CInstantSendDb::CURRENT_VERSION) {
        CDBBatch batch(db);
        CInstantSendLock islock;
        CTransactionRef tx;
        uint256 hashBlock;

        auto it = std::unique_ptr<CDBIterator>(db.NewIterator());
        auto firstKey = std::make_tuple(DB_ISLOCK_BY_HASH, uint256());
        it->Seek(firstKey);
        decltype(firstKey) curKey;

        while (it->Valid()) {
            if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_ISLOCK_BY_HASH) {
                break;
            }
            if (it->GetValue(islock)) {
                if (!GetTransaction(islock.txid, tx, Params().GetConsensus(), hashBlock)) {
                    // Drop locks for unknown txes
                    batch.Erase(std::make_tuple(DB_HASH_BY_TXID, islock.txid));
                    for (auto& in : islock.inputs) {
                        batch.Erase(std::make_tuple(DB_HASH_BY_OUTPOINT, in));
                    }
                    batch.Erase(curKey);
                }
            }
            it->Next();
        }
        batch.Write(DB_VERSION, CInstantSendDb::CURRENT_VERSION);
        db.WriteBatch(batch);
    }
}

void CInstantSendDb::WriteNewInstantSendLock(const uint256& hash, const CInstantSendLock& islock)
{
    CDBBatch batch(db);
    batch.Write(std::make_tuple(std::string(DB_ISLOCK_BY_HASH), hash), islock);
    batch.Write(std::make_tuple(std::string(DB_HASH_BY_TXID), islock.txid), hash);
    for (auto& in : islock.inputs) {
        batch.Write(std::make_tuple(std::string(DB_HASH_BY_OUTPOINT), in), hash);
    }
    db.WriteBatch(batch);

    auto p = std::make_shared<CInstantSendLock>(islock);
    islockCache.insert(hash, p);
    txidCache.insert(islock.txid, hash);
    for (auto& in : islock.inputs) {
        outpointCache.insert(in, hash);
    }
}

void CInstantSendDb::RemoveInstantSendLock(CDBBatch& batch, const uint256& hash, CInstantSendLockPtr islock, bool keep_cache)
{
    if (!islock) {
        islock = GetInstantSendLockByHash(hash, false);
        if (!islock) {
            return;
        }
    }

    batch.Erase(std::make_tuple(std::string(DB_ISLOCK_BY_HASH), hash));
    batch.Erase(std::make_tuple(std::string(DB_HASH_BY_TXID), islock->txid));
    for (auto& in : islock->inputs) {
        batch.Erase(std::make_tuple(std::string(DB_HASH_BY_OUTPOINT), in));
    }

    if (!keep_cache) {
        islockCache.erase(hash);
        txidCache.erase(islock->txid);
        for (auto& in : islock->inputs) {
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
    CDBBatch batch(db);
    WriteInstantSendLockMined(batch, hash, nHeight);
    db.WriteBatch(batch);
}

void CInstantSendDb::WriteInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight)
{
    batch.Write(BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nHeight, hash), true);
}

void CInstantSendDb::RemoveInstantSendLockMined(CDBBatch& batch, const uint256& hash, int nHeight)
{
    batch.Erase(BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nHeight, hash));
}

void CInstantSendDb::WriteInstantSendLockArchived(CDBBatch& batch, const uint256& hash, int nHeight)
{
    batch.Write(BuildInversedISLockKey(DB_ARCHIVED_BY_HEIGHT_AND_HASH, nHeight, hash), true);
    batch.Write(std::make_tuple(std::string(DB_ARCHIVED_BY_HASH), hash), true);
}

std::unordered_map<uint256, CInstantSendLockPtr> CInstantSendDb::RemoveConfirmedInstantSendLocks(int nUntilHeight)
{
    if (nUntilHeight <= 0) {
        return {};
    }

    auto it = std::unique_ptr<CDBIterator>(db.NewIterator());

    auto firstKey = BuildInversedISLockKey(DB_MINED_BY_HEIGHT_AND_HASH, nUntilHeight, uint256());

    it->Seek(firstKey);

    CDBBatch batch(db);
    std::unordered_map<uint256, CInstantSendLockPtr> ret;
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_MINED_BY_HEIGHT_AND_HASH) {
            break;
        }
        uint32_t nHeight = std::numeric_limits<uint32_t>::max() - be32toh(std::get<1>(curKey));
        if (nHeight > nUntilHeight) {
            break;
        }

        auto& islockHash = std::get<2>(curKey);
        auto islock = GetInstantSendLockByHash(islockHash, false);
        if (islock) {
            RemoveInstantSendLock(batch, islockHash, islock);
            ret.emplace(islockHash, islock);
        }

        // archive the islock hash, so that we're still able to check if we've seen the islock in the past
        WriteInstantSendLockArchived(batch, islockHash, nHeight);

        batch.Erase(curKey);

        it->Next();
    }

    db.WriteBatch(batch);

    return ret;
}

void CInstantSendDb::RemoveArchivedInstantSendLocks(int nUntilHeight)
{
    if (nUntilHeight <= 0) {
        return;
    }

    auto it = std::unique_ptr<CDBIterator>(db.NewIterator());

    auto firstKey = BuildInversedISLockKey(DB_ARCHIVED_BY_HEIGHT_AND_HASH, nUntilHeight, uint256());

    it->Seek(firstKey);

    CDBBatch batch(db);
    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_ARCHIVED_BY_HEIGHT_AND_HASH) {
            break;
        }
        uint32_t nHeight = std::numeric_limits<uint32_t>::max() - be32toh(std::get<1>(curKey));
        if (nHeight > nUntilHeight) {
            break;
        }

        auto& islockHash = std::get<2>(curKey);
        batch.Erase(std::make_tuple(std::string(DB_ARCHIVED_BY_HASH), islockHash));
        batch.Erase(curKey);

        it->Next();
    }

    db.WriteBatch(batch);
}

void CInstantSendDb::WriteBlockInstantSendLocks(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected)
{
    CDBBatch batch(db);
    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            // coinbase and TXs with no inputs can't be locked
            continue;
        }
        uint256 islockHash = GetInstantSendLockHashByTxid(tx->GetHash());
        // update DB about when an IS lock was mined
        if (!islockHash.IsNull()) {
            WriteInstantSendLockMined(batch, islockHash, pindexConnected->nHeight);
        }
    }
    db.WriteBatch(batch);
}

void CInstantSendDb::RemoveBlockInstantSendLocks(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    CDBBatch batch(db);
    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            // coinbase and TXs with no inputs can't be locked
            continue;
        }
        uint256 islockHash = GetInstantSendLockHashByTxid(tx->GetHash());
        if (!islockHash.IsNull()) {
            RemoveInstantSendLockMined(batch, islockHash, pindexDisconnected->nHeight);
        }
    }
    db.WriteBatch(batch);
}

bool CInstantSendDb::KnownInstantSendLock(const uint256& islockHash) const
{
    return GetInstantSendLockByHash(islockHash) != nullptr || db.Exists(std::make_tuple(std::string(DB_ARCHIVED_BY_HASH), islockHash));
}

size_t CInstantSendDb::GetInstantSendLockCount() const
{
    auto it = std::unique_ptr<CDBIterator>(db.NewIterator());
    auto firstKey = std::make_tuple(std::string(DB_ISLOCK_BY_HASH), uint256());

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

CInstantSendLockPtr CInstantSendDb::GetInstantSendLockByHash(const uint256& hash, bool use_cache) const
{
    if (hash.IsNull()) {
        return nullptr;
    }

    CInstantSendLockPtr ret;
    if (use_cache && islockCache.get(hash, ret)) {
        return ret;
    }

    ret = std::make_shared<CInstantSendLock>();
    bool exists = db.Read(std::make_tuple(std::string(DB_ISLOCK_BY_HASH), hash), *ret);
    if (!exists) {
        ret = nullptr;
    }
    islockCache.insert(hash, ret);
    return ret;
}

uint256 CInstantSendDb::GetInstantSendLockHashByTxid(const uint256& txid) const
{
    uint256 islockHash;
    if (!txidCache.get(txid, islockHash)) {
        db.Read(std::make_tuple(std::string(DB_HASH_BY_TXID), txid), islockHash);
        txidCache.insert(txid, islockHash);
    }
    return islockHash;
}

CInstantSendLockPtr CInstantSendDb::GetInstantSendLockByTxid(const uint256& txid) const
{
    return GetInstantSendLockByHash(GetInstantSendLockHashByTxid(txid));
}

CInstantSendLockPtr CInstantSendDb::GetInstantSendLockByInput(const COutPoint& outpoint) const
{
    uint256 islockHash;
    if (!outpointCache.get(outpoint, islockHash)) {
        db.Read(std::make_tuple(std::string(DB_HASH_BY_OUTPOINT), outpoint), islockHash);
        outpointCache.insert(outpoint, islockHash);
    }
    return GetInstantSendLockByHash(islockHash);
}

std::vector<uint256> CInstantSendDb::GetInstantSendLocksByParent(const uint256& parent) const
{
    auto it = std::unique_ptr<CDBIterator>(db.NewIterator());
    auto firstKey = std::make_tuple(std::string(DB_HASH_BY_OUTPOINT), COutPoint(parent, 0));
    it->Seek(firstKey);

    std::vector<uint256> result;

    while (it->Valid()) {
        decltype(firstKey) curKey;
        if (!it->GetKey(curKey) || std::get<0>(curKey) != DB_HASH_BY_OUTPOINT) {
            break;
        }
        auto& outpoint = std::get<1>(curKey);
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
    std::vector<uint256> result;

    std::vector<uint256> stack;
    std::unordered_set<uint256, StaticSaltedHasher> added;
    stack.emplace_back(txid);

    CDBBatch batch(db);
    while (!stack.empty()) {
        auto children = GetInstantSendLocksByParent(stack.back());
        stack.pop_back();

        for (auto& childIslockHash : children) {
            auto childIsLock = GetInstantSendLockByHash(childIslockHash, false);
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

    db.WriteBatch(batch);

    return result;
}

////////////////

CInstantSendManager::CInstantSendManager(CDBWrapper& _llmqDb) :
    db(_llmqDb)
{
    workInterrupt.reset();
}

CInstantSendManager::~CInstantSendManager() = default;

void CInstantSendManager::Start()
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&TraceThread<std::function<void()> >, "isman", std::function<void()>(std::bind(&CInstantSendManager::WorkThreadMain, this)));

    quorumSigningManager->RegisterRecoveredSigsListener(this);
}

void CInstantSendManager::Stop()
{
    quorumSigningManager->UnregisterRecoveredSigsListener(this);

    // make sure to call InterruptWorkerThread() first
    if (!workInterrupt) {
        assert(false);
    }

    if (workThread.joinable()) {
        workThread.join();
    }
}

void CInstantSendManager::InterruptWorkerThread()
{
    workInterrupt();
}

void CInstantSendManager::ProcessTx(const CTransaction& tx, bool fRetroactive, const Consensus::Params& params)
{
    if (!fMasternodeMode || !IsInstantSendEnabled() || !masternodeSync.IsBlockchainSynced()) {
        return;
    }

    auto llmqType = params.llmqTypeInstantSend;
    if (llmqType == Consensus::LLMQ_NONE) {
        return;
    }

    // In case the islock was received before the TX, filtered announcement might have missed this islock because
    // we were unable to check for filter matches deep inside the TX. Now we have the TX, so we should retry.
    uint256 islockHash;
    {
        LOCK(cs);
        islockHash = db.GetInstantSendLockHashByTxid(tx.GetHash());
    }
    if (!islockHash.IsNull()) {
        CInv inv(MSG_ISLOCK, islockHash);
        g_connman->RelayInvFiltered(inv, tx, LLMQS_PROTO_VERSION);
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

    if (!TrySignInputLocks(tx, fRetroactive, llmqType)) return;

    // We might have received all input locks before we got the corresponding TX. In this case, we have to sign the
    // islock now instead of waiting for the input locks.
    TrySignInstantSendLock(tx);
}

bool CInstantSendManager::TrySignInputLocks(const CTransaction& tx, bool fRetroactive, Consensus::LLMQType llmqType)
{
    std::vector<uint256> ids;
    ids.reserve(tx.vin.size());

    size_t alreadyVotedCount = 0;
    for (const auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        ids.emplace_back(id);

        uint256 otherTxHash;
        if (quorumSigningManager->GetVoteForId(llmqType, id, otherTxHash)) {
            if (otherTxHash != tx.GetHash()) {
                LogPrintf("CInstantSendManager::%s -- txid=%s: input %s is conflicting with previous vote for tx %s\n", __func__,
                          tx.GetHash().ToString(), in.prevout.ToStringShort(), otherTxHash.ToString());
                return false;
            }
            alreadyVotedCount++;
        }

        // don't even try the actual signing if any input is conflicting
        if (quorumSigningManager->IsConflicting(llmqType, id, tx.GetHash())) {
            LogPrintf("CInstantSendManager::%s -- txid=%s: quorumSigningManager->IsConflicting returned true. id=%s\n", __func__,
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

    for (size_t i = 0; i < tx.vin.size(); i++) {
        auto& in = tx.vin[i];
        auto& id = ids[i];
        {
            LOCK(cs);
            inputRequestIds.emplace(id);
        }
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: trying to vote on input %s with id %s. fRetroactive=%d\n", __func__,
                 tx.GetHash().ToString(), in.prevout.ToStringShort(), id.ToString(), fRetroactive);
        if (quorumSigningManager->AsyncSignIfMember(llmqType, id, tx.GetHash(), {}, fRetroactive)) {
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

    for (const auto& in : tx.vin) {
        CAmount v = 0;
        if (!CheckCanLock(in.prevout, printDebug, tx.GetHash(), &v, params)) {
            return false;
        }
    }

    return true;
}

bool CInstantSendManager::CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256& txHash, CAmount* retValue, const Consensus::Params& params) const
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

    CTransactionRef tx;
    uint256 hashBlock;
    // this relies on enabled txindex and won't work if we ever try to remove the requirement for txindex for masternodes
    if (!GetTransaction(outpoint.hash, tx, params, hashBlock, false)) {
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
        pindexMined = LookupBlockIndex(hashBlock);
        nTxAge = chainActive.Height() - pindexMined->nHeight + 1;
    }

    if (nTxAge < nInstantSendConfirmationsRequired) {
        if (!llmq::chainLocksHandler->HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
            if (printDebug) {
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: outpoint %s too new and not ChainLocked. nTxAge=%d, nInstantSendConfirmationsRequired=%d\n", __func__,
                         txHash.ToString(), outpoint.ToStringShort(), nTxAge, nInstantSendConfirmationsRequired);
            }
            return false;
        }
    }

    if (retValue) {
        *retValue = tx->vout[outpoint.n].nValue;
    }

    return true;
}

void CInstantSendManager::HandleNewRecoveredSig(const CRecoveredSig& recoveredSig)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    auto llmqType = Params().GetConsensus().llmqTypeInstantSend;
    if (llmqType == Consensus::LLMQ_NONE) {
        return;
    }
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    uint256 txid;
    bool isInstantSendLock = false;
    {
        LOCK(cs);
        if (inputRequestIds.count(recoveredSig.id)) {
            txid = recoveredSig.msgHash;
        }
        if (creatingInstantSendLocks.count(recoveredSig.id)) {
            isInstantSendLock = true;
        }
    }
    if (!txid.IsNull()) {
        HandleNewInputLockRecoveredSig(recoveredSig, txid);
    } else if (isInstantSendLock) {
        HandleNewInstantSendLockRecoveredSig(recoveredSig);
    }
}

void CInstantSendManager::HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid)
{
    CTransactionRef tx;
    uint256 hashBlock;
    if (!GetTransaction(txid, tx, Params().GetConsensus(), hashBlock, true)) {
        return;
    }

    if (LogAcceptCategory(BCLog::INSTANTSEND)) {
        for (auto& in : tx->vin) {
            auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
            if (id == recoveredSig.id) {
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
    auto llmqType = Params().GetConsensus().llmqTypeInstantSend;

    for (auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        if (!quorumSigningManager->HasRecoveredSig(llmqType, id, tx.GetHash())) {
            return;
        }
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: got all recovered sigs, creating CInstantSendLock\n", __func__,
            tx.GetHash().ToString());

    CInstantSendLock islock;
    islock.txid = tx.GetHash();
    for (auto& in : tx.vin) {
        islock.inputs.emplace_back(in.prevout);
    }

    auto id = islock.GetRequestId();

    if (quorumSigningManager->HasRecoveredSigForId(llmqType, id)) {
        return;
    }

    {
        LOCK(cs);
        auto e = creatingInstantSendLocks.emplace(id, std::move(islock));
        if (!e.second) {
            return;
        }
        txToCreatingInstantSendLocks.emplace(tx.GetHash(), &e.first->second);
    }

    quorumSigningManager->AsyncSignIfMember(llmqType, id, tx.GetHash());
}

void CInstantSendManager::HandleNewInstantSendLockRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    CInstantSendLockPtr islock;

    {
        LOCK(cs);
        auto it = creatingInstantSendLocks.find(recoveredSig.id);
        if (it == creatingInstantSendLocks.end()) {
            return;
        }

        islock = std::make_shared<CInstantSendLock>(std::move(it->second));
        creatingInstantSendLocks.erase(it);
        txToCreatingInstantSendLocks.erase(islock->txid);
    }

    if (islock->txid != recoveredSig.msgHash) {
        LogPrintf("CInstantSendManager::%s -- txid=%s: islock conflicts with %s, dropping own version\n", __func__,
                islock->txid.ToString(), recoveredSig.msgHash.ToString());
        return;
    }

    islock->sig = recoveredSig.sig;
    ProcessInstantSendLock(-1, ::SerializeHash(*islock), islock);
}

void CInstantSendManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    if (strCommand == NetMsgType::ISLOCK) {
        CInstantSendLockPtr islock = std::make_shared<CInstantSendLock>();
        vRecv >> *islock;
        ProcessMessageInstantSendLock(pfrom, islock);
    }
}

void CInstantSendManager::ProcessMessageInstantSendLock(CNode* pfrom, const llmq::CInstantSendLockPtr& islock)
{
    auto hash = ::SerializeHash(*islock);

    {
        LOCK(cs_main);
        EraseObjectRequest(pfrom->GetId(), CInv(MSG_ISLOCK, hash));
    }

    if (!PreVerifyInstantSendLock(*islock)) {
        LOCK(cs_main);
        Misbehaving(pfrom->GetId(), 100);
        return;
    }

    LOCK(cs);
    if (pendingInstantSendLocks.count(hash) || db.KnownInstantSendLock(hash)) {
        return;
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: received islock, peer=%d\n", __func__,
            islock->txid.ToString(), hash.ToString(), pfrom->GetId());

    pendingInstantSendLocks.emplace(hash, std::make_pair(pfrom->GetId(), islock));
}

/**
 * Handles trivial ISLock verification
 * @param islock The islock message being undergoing verification
 * @return returns false if verification failed, otherwise true
 */
bool CInstantSendManager::PreVerifyInstantSendLock(const llmq::CInstantSendLock& islock)
{
    if (islock.txid.IsNull() || islock.inputs.empty()) {
        return false;
    }

    // Check that each input is unique
    std::set<COutPoint> dups;
    for (auto& o : islock.inputs) {
        if (!dups.emplace(o).second) {
            return false;
        }
    }

    return true;
}

bool CInstantSendManager::ProcessPendingInstantSendLocks()
{
    decltype(pendingInstantSendLocks) pend;
    bool fMoreWork{false};

    {
        LOCK(cs);
        // only process a max 32 locks at a time to avoid duplicate verification of recovered signatures which have been
        // verified by CSigningManager in parallel
        const size_t maxCount = 32;
        if (pendingInstantSendLocks.size() <= maxCount) {
            pend = std::move(pendingInstantSendLocks);
        } else {
            while (pend.size() < maxCount) {
                auto it = pendingInstantSendLocks.begin();
                pend.emplace(it->first, std::move(it->second));
                pendingInstantSendLocks.erase(it);
            }
            fMoreWork = true;
        }
    }

    if (pend.empty()) {
        return false;
    }

    if (!IsInstantSendEnabled()) {
        return false;
    }

    auto llmqType = Params().GetConsensus().llmqTypeInstantSend;
    auto dkgInterval = Params().GetConsensus().llmqs.at(llmqType).dkgInterval;

    // First check against the current active set and don't ban
    auto badISLocks = ProcessPendingInstantSendLocks(0, pend, false);
    if (!badISLocks.empty()) {
        LogPrintf("CInstantSendManager::%s -- doing verification on old active set\n", __func__);

        // filter out valid IS locks from "pend"
        for (auto it = pend.begin(); it != pend.end(); ) {
            if (!badISLocks.count(it->first)) {
                it = pend.erase(it);
            } else {
                ++it;
            }
        }
        // Now check against the previous active set and perform banning if this fails
        ProcessPendingInstantSendLocks(dkgInterval, pend, true);
    }

    return fMoreWork;
}

std::unordered_set<uint256> CInstantSendManager::ProcessPendingInstantSendLocks(int signOffset, const std::unordered_map<uint256, std::pair<NodeId, CInstantSendLockPtr>, StaticSaltedHasher>& pend, bool ban)
{
    auto llmqType = Params().GetConsensus().llmqTypeInstantSend;

    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, true, 8);
    std::unordered_map<uint256, CRecoveredSig> recSigs;

    size_t verifyCount = 0;
    size_t alreadyVerified = 0;
    for (const auto& p : pend) {
        auto& hash = p.first;
        auto nodeId = p.second.first;
        auto& islock = p.second.second;

        if (batchVerifier.badSources.count(nodeId)) {
            continue;
        }

        if (!islock->sig.Get().IsValid()) {
            batchVerifier.badSources.emplace(nodeId);
            continue;
        }

        auto id = islock->GetRequestId();

        // no need to verify an ISLOCK if we already have verified the recovered sig that belongs to it
        if (quorumSigningManager->HasRecoveredSig(llmqType, id, islock->txid)) {
            alreadyVerified++;
            continue;
        }

        auto quorum = quorumSigningManager->SelectQuorumForSigning(llmqType, id, -1, signOffset);
        if (!quorum) {
            // should not happen, but if one fails to select, all others will also fail to select
            return {};
        }
        uint256 signHash = CLLMQUtils::BuildSignHash(llmqType, quorum->qc.quorumHash, id, islock->txid);
        batchVerifier.PushMessage(nodeId, hash, signHash, islock->sig.Get(), quorum->qc.quorumPublicKey);
        verifyCount++;

        // We can reconstruct the CRecoveredSig objects from the islock and pass it to the signing manager, which
        // avoids unnecessary double-verification of the signature. We however only do this when verification here
        // turns out to be good (which is checked further down)
        if (!quorumSigningManager->HasRecoveredSigForId(llmqType, id)) {
            CRecoveredSig recSig;
            recSig.llmqType = llmqType;
            recSig.quorumHash = quorum->qc.quorumHash;
            recSig.id = id;
            recSig.msgHash = islock->txid;
            recSig.sig = islock->sig;
            recSigs.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(std::move(recSig)));
        }
    }

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- verified locks. count=%d, alreadyVerified=%d, vt=%d, nodes=%d\n", __func__,
            verifyCount, alreadyVerified, verifyTimer.count(), batchVerifier.GetUniqueSourceCount());

    std::unordered_set<uint256> badISLocks;

    if (ban && !batchVerifier.badSources.empty()) {
        LOCK(cs_main);
        for (auto& nodeId : batchVerifier.badSources) {
            // Let's not be too harsh, as the peer might simply be unlucky and might have sent us an old lock which
            // does not validate anymore due to changed quorums
            Misbehaving(nodeId, 20);
        }
    }
    for (const auto& p : pend) {
        auto& hash = p.first;
        auto nodeId = p.second.first;
        auto& islock = p.second.second;

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
            std::shared_ptr<CRecoveredSig> recSig = std::make_shared<CRecoveredSig>(std::move(it->second));
            if (!quorumSigningManager->HasRecoveredSigForId(llmqType, recSig->id)) {
                recSig->UpdateHash();
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: passing reconstructed recSig to signing mgr, peer=%d\n", __func__,
                         islock->txid.ToString(), hash.ToString(), nodeId);
                quorumSigningManager->PushReconstructedRecoveredSig(recSig);
            }
        }
    }

    return badISLocks;
}

void CInstantSendManager::ProcessInstantSendLock(NodeId from, const uint256& hash, const CInstantSendLockPtr& islock)
{
    {
        LOCK(cs);

        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: processsing islock, peer=%d\n", __func__,
                 islock->txid.ToString(), hash.ToString(), from);

        creatingInstantSendLocks.erase(islock->GetRequestId());
        txToCreatingInstantSendLocks.erase(islock->txid);

        if (db.KnownInstantSendLock(hash)) {
            return;
        }
    }

    CTransactionRef tx;
    uint256 hashBlock;
    const CBlockIndex* pindexMined{nullptr};
    // we ignore failure here as we must be able to propagate the lock even if we don't have the TX locally
    if (GetTransaction(islock->txid, tx, Params().GetConsensus(), hashBlock) && !hashBlock.IsNull()) {
        {
            LOCK(cs_main);
            pindexMined = LookupBlockIndex(hashBlock);
        }

        // Let's see if the TX that was locked by this islock is already mined in a ChainLocked block. If yes,
        // we can simply ignore the islock, as the ChainLock implies locking of all TXs in that chain
        if (pindexMined != nullptr && llmq::chainLocksHandler->HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txlock=%s, islock=%s: dropping islock as it already got a ChainLock in block %s, peer=%d\n", __func__,
                     islock->txid.ToString(), hash.ToString(), hashBlock.ToString(), from);
            return;
        }
    }

    {
        LOCK(cs);
        CInstantSendLockPtr otherIsLock = db.GetInstantSendLockByTxid(islock->txid);
        if (otherIsLock != nullptr) {
            LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: duplicate islock, other islock=%s, peer=%d\n", __func__,
                     islock->txid.ToString(), hash.ToString(), ::SerializeHash(*otherIsLock).ToString(), from);
        }
        for (auto& in : islock->inputs) {
            otherIsLock = db.GetInstantSendLockByInput(in);
            if (otherIsLock != nullptr) {
                LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: conflicting input in islock. input=%s, other islock=%s, peer=%d\n", __func__,
                         islock->txid.ToString(), hash.ToString(), in.ToStringShort(), ::SerializeHash(*otherIsLock).ToString(), from);
            }
        }

        db.WriteNewInstantSendLock(hash, *islock);
        if (pindexMined) {
            db.WriteInstantSendLockMined(hash, pindexMined->nHeight);
        }

        // This will also add children TXs to pendingRetryTxs
        RemoveNonLockedTx(islock->txid, true);

        // We don't need the recovered sigs for the inputs anymore. This prevents unnecessary propagation of these sigs.
        // We only need the ISLOCK from now on to detect conflicts
        TruncateRecoveredSigsForInputs(*islock);
    }

    CInv inv(MSG_ISLOCK, hash);
    if (tx != nullptr) {
        g_connman->RelayInvFiltered(inv, *tx, LLMQS_PROTO_VERSION);
    } else {
        // we don't have the TX yet, so we only filter based on txid. Later when that TX arrives, we will re-announce
        // with the TX taken into account.
        g_connman->RelayInvFiltered(inv, islock->txid, LLMQS_PROTO_VERSION);
    }

    ResolveBlockConflicts(hash, *islock);
    RemoveMempoolConflictsForLock(hash, *islock);

    if (tx != nullptr) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- notify about an in-time lock for tx %s\n", __func__, tx->GetHash().ToString());
        GetMainSignals().NotifyTransactionLock(tx, islock);
        // bump mempool counter to make sure newly locked txes are picked up by getblocktemplate
        mempool.AddTransactionsUpdated(1);
    }
}

void CInstantSendManager::TransactionAddedToMempool(const CTransactionRef& tx)
{
    if (!IsInstantSendEnabled() || !masternodeSync.IsBlockchainSynced() || tx->vin.empty()) {
        return;
    }

    CInstantSendLockPtr islock{nullptr};
    {
        LOCK(cs);
        islock = db.GetInstantSendLockByTxid(tx->GetHash());
    }

    if (islock == nullptr) {
        ProcessTx(*tx, false, Params().GetConsensus());
        // TX is not locked, so make sure it is tracked
        LOCK(cs);
        AddNonLockedTx(tx, nullptr);
    } else {
        {
            // TX is locked, so make sure we don't track it anymore
            LOCK(cs);
            RemoveNonLockedTx(tx->GetHash(), true);
        }
        // If the islock was received before the TX, we know we were not able to send
        // the notification at that time, we need to do it now.
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- notify about an earlier received lock for tx %s\n", __func__, tx->GetHash().ToString());
        GetMainSignals().NotifyTransactionLock(tx, islock);
    }
}

void CInstantSendManager::TransactionRemovedFromMempool(const CTransactionRef& tx)
{
    if (tx->vin.empty() || !fUpgradedDB) {
        return;
    }

    LOCK(cs);
    CInstantSendLockPtr islock = db.GetInstantSendLockByTxid(tx->GetHash());

    if (islock == nullptr) {
        return;
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- transaction %s was removed from mempool\n", __func__, tx->GetHash().ToString());
    RemoveConflictingLock(::SerializeHash(*islock), *islock);
}

void CInstantSendManager::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    if (!vtxConflicted.empty()) {
        LOCK(cs);
        for (const auto& tx : vtxConflicted) {
            RemoveConflictedTx(*tx);
        }
    }

    if (masternodeSync.IsBlockchainSynced()) {
        for (const auto& tx : pblock->vtx) {
            if (tx->IsCoinBase() || tx->vin.empty()) {
                // coinbase and TXs with no inputs can't be locked
                continue;
            }

            if (!IsLocked(tx->GetHash()) && !chainLocksHandler->HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
                ProcessTx(*tx, true, Params().GetConsensus());
                // TX is not locked, so make sure it is tracked
                LOCK(cs);
                AddNonLockedTx(tx, pindex);
            } else {
                // TX is locked, so make sure we don't track it anymore
                LOCK(cs);
                RemoveNonLockedTx(tx->GetHash(), true);
            }
        }
    }

    LOCK(cs);
    db.WriteBlockInstantSendLocks(pblock, pindex);
}

void CInstantSendManager::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    LOCK(cs);
    db.RemoveBlockInstantSendLocks(pblock, pindexDisconnected);
}

void CInstantSendManager::AddNonLockedTx(const CTransactionRef& tx, const CBlockIndex* pindexMined)
{
    AssertLockHeld(cs);
    auto res = nonLockedTxs.emplace(tx->GetHash(), NonLockedTxInfo());
    auto& info = res.first->second;
    info.pindexMined = pindexMined;

    if (res.second) {
        info.tx = tx;
        for (const auto& in : tx->vin) {
            nonLockedTxs[in.prevout.hash].children.emplace(tx->GetHash());
            nonLockedTxsByOutpoints.emplace(in.prevout, tx->GetHash());
        }
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, pindexMined=%s\n", __func__,
             tx->GetHash().ToString(), pindexMined ? pindexMined->GetBlockHash().ToString() : "");
}

void CInstantSendManager::RemoveNonLockedTx(const uint256& txid, bool retryChildren)
{
    AssertLockHeld(cs);

    auto it = nonLockedTxs.find(txid);
    if (it == nonLockedTxs.end()) {
        return;
    }
    auto& info = it->second;

    size_t retryChildrenCount = 0;
    if (retryChildren) {
        // TX got locked, so we can retry locking children
        for (auto& childTxid : info.children) {
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
    AssertLockHeld(cs);
    RemoveNonLockedTx(tx.GetHash(), false);

    for (const auto& in : tx.vin) {
        auto inputRequestId = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in));
        inputRequestIds.erase(inputRequestId);
    }
}

void CInstantSendManager::TruncateRecoveredSigsForInputs(const llmq::CInstantSendLock& islock)
{
    AssertLockHeld(cs);
    auto& consensusParams = Params().GetConsensus();

    for (auto& in : islock.inputs) {
        auto inputRequestId = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in));
        inputRequestIds.erase(inputRequestId);
        quorumSigningManager->TruncateRecoveredSig(consensusParams.llmqTypeInstantSend, inputRequestId);
    }
}

void CInstantSendManager::NotifyChainLock(const CBlockIndex* pindexChainLock)
{
    HandleFullyConfirmedBlock(pindexChainLock);
}

void CInstantSendManager::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    if (!fUpgradedDB) {
        LOCK(cs_main);
        if (VersionBitsState(pindexNew, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0020, versionbitscache) == ThresholdState::ACTIVE) {
            db.Upgrade();
            fUpgradedDB = true;
        }
    }

    bool fDIP0008Active = pindexNew->pprev && pindexNew->pprev->nHeight >= Params().GetConsensus().DIP0008Height;

    if (AreChainLocksEnabled() && fDIP0008Active) {
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

    LOCK(cs);

    auto& consensusParams = Params().GetConsensus();
    auto removeISLocks = db.RemoveConfirmedInstantSendLocks(pindex->nHeight);

    for (auto& p : removeISLocks) {
        auto& islockHash = p.first;
        auto& islock = p.second;
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: removed islock as it got fully confirmed\n", __func__,
                 islock->txid.ToString(), islockHash.ToString());

        // No need to keep recovered sigs for fully confirmed IS locks, as there is no chance for conflicts
        // from now on. All inputs are spent now and can't be spend in any other TX.
        TruncateRecoveredSigsForInputs(*islock);

        // And we don't need the recovered sig for the ISLOCK anymore, as the block in which it got mined is considered
        // fully confirmed now
        quorumSigningManager->TruncateRecoveredSig(consensusParams.llmqTypeInstantSend, islock->GetRequestId());
    }

    db.RemoveArchivedInstantSendLocks(pindex->nHeight - 100);

    // Find all previously unlocked TXs that got locked by this fully confirmed (ChainLock) block and remove them
    // from the nonLockedTxs map. Also collect all children of these TXs and mark them for retrying of IS locking.
    std::vector<uint256> toRemove;
    for (auto& p : nonLockedTxs) {
        auto pindexMined = p.second.pindexMined;

        if (pindexMined && pindex->GetAncestor(pindexMined->nHeight) == pindexMined) {
            toRemove.emplace_back(p.first);
        }
    }
    for (auto& txid : toRemove) {
        // This will also add children to pendingRetryTxs
        RemoveNonLockedTx(txid, true);
    }
}

void CInstantSendManager::RemoveMempoolConflictsForLock(const uint256& hash, const CInstantSendLock& islock)
{
    std::unordered_map<uint256, CTransactionRef> toDelete;

    {
        LOCK(mempool.cs);

        for (auto& in : islock.inputs) {
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

        for (auto& p : toDelete) {
            mempool.removeRecursive(*p.second, MemPoolRemovalReason::CONFLICT);
        }
    }

    if (!toDelete.empty()) {
        {
            LOCK(cs);
            for (auto& p : toDelete) {
                RemoveConflictedTx(*p.second);
            }
        }
        AskNodesForLockedTx(islock.txid);
    }
}

void CInstantSendManager::ResolveBlockConflicts(const uint256& islockHash, const llmq::CInstantSendLock& islock)
{
    // Lets first collect all non-locked TXs which conflict with the given ISLOCK
    std::unordered_map<const CBlockIndex*, std::unordered_map<uint256, CTransactionRef, StaticSaltedHasher>> conflicts;
    {
        LOCK(cs);
        for (auto& in : islock.inputs) {
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
        auto pindex = p.first;
        if (chainLocksHandler->HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
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

    bool activateBestChain = false;
    for (const auto& p : conflicts) {
        auto pindex = p.first;
        {
            LOCK(cs);
            for (auto& p2 : p.second) {
                const auto& tx = *p2.second;
                RemoveConflictedTx(tx);
            }
        }

        LogPrintf("CInstantSendManager::%s -- invalidating block %s\n", __func__, pindex->GetBlockHash().ToString());

        LOCK(cs_main);
        CValidationState state;
        // need non-const pointer
        auto pindex2 = LookupBlockIndex(pindex->GetBlockHash());
        if (!InvalidateBlock(state, Params(), pindex2)) {
            LogPrintf("CInstantSendManager::%s -- InvalidateBlock failed: %s\n", __func__, FormatStateMessage(state));
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
        activateBestChain = true;
    }

    if (activateBestChain) {
        CValidationState state;
        if (!ActivateBestChain(state, Params())) {
            LogPrintf("CChainLocksHandler::%s -- ActivateBestChain failed: %s\n", __func__, FormatStateMessage(state));
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
    }
}

void CInstantSendManager::RemoveConflictingLock(const uint256& islockHash, const llmq::CInstantSendLock& islock)
{
    LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: Removing ISLOCK and its chained children\n", __func__,
              islock.txid.ToString(), islockHash.ToString());
    int tipHeight;
    {
        LOCK(cs_main);
        tipHeight = chainActive.Height();
    }

    LOCK(cs);
    auto removedIslocks = db.RemoveChainedInstantSendLocks(islockHash, islock.txid, tipHeight);
    for (auto& h : removedIslocks) {
        LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: removed (child) ISLOCK %s\n", __func__,
                  islock.txid.ToString(), islockHash.ToString(), h.ToString());
    }
}

void CInstantSendManager::AskNodesForLockedTx(const uint256& txid)
{
    std::vector<CNode*> nodesToAskFor;
    g_connman->ForEachNode([&](CNode* pnode) {
        LOCK(pnode->cs_filter);
        if (pnode->filterInventoryKnown.contains(txid)) {
            pnode->AddRef();
            nodesToAskFor.emplace_back(pnode);
        }
    });
    {
        LOCK(cs_main);
        for (CNode* pnode : nodesToAskFor) {
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
    decltype(pendingRetryTxs) retryTxs;
    {
        LOCK(cs);
        retryTxs = std::move(pendingRetryTxs);
    }

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
            LOCK(cs);
            auto it = nonLockedTxs.find(txid);
            if (it == nonLockedTxs.end()) {
                continue;
            }
            tx = it->second.tx;

            if (!tx) {
                continue;
            }

            if (txToCreatingInstantSendLocks.count(tx->GetHash())) {
                // we're already in the middle of locking this one
                continue;
            }
            if (IsLocked(tx->GetHash())) {
                continue;
            }
            if (IsConflicted(*tx)) {
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
        LOCK(cs);
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- retried %d TXs. nonLockedTxs.size=%d\n", __func__,
                 retryCount, nonLockedTxs.size());
    }
}

bool CInstantSendManager::AlreadyHave(const CInv& inv) const
{
    if (!IsInstantSendEnabled()) {
        return true;
    }

    LOCK(cs);
    return pendingInstantSendLocks.count(inv.hash) != 0 || db.KnownInstantSendLock(inv.hash);
}

bool CInstantSendManager::GetInstantSendLockByHash(const uint256& hash, llmq::CInstantSendLock& ret) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    LOCK(cs);
    auto islock = db.GetInstantSendLockByHash(hash);
    if (!islock) {
        return false;
    }
    ret = *islock;
    return true;
}

CInstantSendLockPtr CInstantSendManager::GetInstantSendLockByTxid(const uint256& txid) const
{
    if (!IsInstantSendEnabled()) {
        return nullptr;
    }

    LOCK(cs);
    return db.GetInstantSendLockByTxid(txid);
}

bool CInstantSendManager::GetInstantSendLockHashByTxid(const uint256& txid, uint256& ret) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    LOCK(cs);
    ret = db.GetInstantSendLockHashByTxid(txid);
    return !ret.IsNull();
}

bool CInstantSendManager::IsLocked(const uint256& txHash) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    LOCK(cs);
    return db.KnownInstantSendLock(db.GetInstantSendLockHashByTxid(txHash));
}

bool CInstantSendManager::IsConflicted(const CTransaction& tx) const
{
    return GetConflictingLock(tx) != nullptr;
}

CInstantSendLockPtr CInstantSendManager::GetConflictingLock(const CTransaction& tx) const
{
    if (!IsInstantSendEnabled()) {
        return nullptr;
    }

    LOCK(cs);
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

bool IsInstantSendEnabled()
{
    return !fReindex && !fImporting && sporkManager.IsSporkActive(SPORK_2_INSTANTSEND_ENABLED);
}

bool IsInstantSendMempoolSigningEnabled()
{
    return !fReindex && !fImporting && sporkManager.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED) == 0;
}

bool RejectConflictingBlocks()
{
    return !fReindex && !fImporting && sporkManager.IsSporkActive(SPORK_3_INSTANTSEND_BLOCK_FILTERING);
}

} // namespace llmq
