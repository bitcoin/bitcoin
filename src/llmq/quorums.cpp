// Copyright (c) 2018-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_dkgsession.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_init.h>
#include <llmq/quorums_utils.h>

#include <special/specialtx.h>

#include <masternodes/activemasternode.h>
#include <chainparams.h>
#include <init.h>
#include <masternodes/sync.h>
#include <shutdown.h>
#include <univalue.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq
{

static const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
static const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

CQuorumManager* quorumManager;

static uint256 MakeQuorumKey(const CQuorum& q)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << (uint8_t)q.params.type;
    hw << q.qc.quorumHash;
    for (const auto& dmn : q.members) {
        hw << dmn->proTxHash;
    }
    return hw.GetHash();
}

CQuorum::~CQuorum()
{
    // most likely the thread is already done
    stopCachePopulatorThread = true;
    if (cachePopulatorThread.joinable()) {
        cachePopulatorThread.join();
    }
}

void CQuorum::Init(const CFinalCommitment& _qc, const CBlockIndex* _pindexQuorum, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members)
{
    qc = _qc;
    pindexQuorum = _pindexQuorum;
    members = _members;
    minedBlockHash = _minedBlockHash;
}

bool CQuorum::IsMember(const uint256& proTxHash) const
{
    for (auto& dmn : members) {
        if (dmn->proTxHash == proTxHash) {
            return true;
        }
    }
    return false;
}

bool CQuorum::IsValidMember(const uint256& proTxHash) const
{
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i]->proTxHash == proTxHash) {
            return qc.validMembers[i];
        }
    }
    return false;
}

CBLSPublicKey CQuorum::GetPubKeyShare(size_t memberIdx) const
{
    if (quorumVvec == nullptr || memberIdx >= members.size() || !qc.validMembers[memberIdx]) {
        return CBLSPublicKey();
    }
    auto& m = members[memberIdx];
    return blsCache.BuildPubKeyShare(m->proTxHash, quorumVvec, CBLSId::FromHash(m->proTxHash));
}

CBLSSecretKey CQuorum::GetSkShare() const
{
    return skShare;
}

int CQuorum::GetMemberIndex(const uint256& proTxHash) const
{
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i]->proTxHash == proTxHash) {
            return (int)i;
        }
    }
    return -1;
}

void CQuorum::WriteContributions(CSpecialDB& specialDb)
{
    uint256 dbKey = MakeQuorumKey(*this);

    if (quorumVvec != nullptr) {
        specialDb.GetRawDB().Write(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), *quorumVvec);
    }
    if (skShare.IsValid()) {
        specialDb.GetRawDB().Write(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);
    }
}

bool CQuorum::ReadContributions(CSpecialDB& specialDb)
{
    uint256 dbKey = MakeQuorumKey(*this);

    BLSVerificationVector qv;
    if (specialDb.Read(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), qv)) {
        quorumVvec = std::make_shared<BLSVerificationVector>(std::move(qv));
    } else {
        return false;
    }

    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    specialDb.Read(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);

    return true;
}

void CQuorum::StartCachePopulatorThread(std::shared_ptr<CQuorum> _this)
{
    if (_this->quorumVvec == nullptr) {
        return;
    }

    cxxtimer::Timer t(true);
    LogPrint(BCLog::LLMQ, "CQuorum::StartCachePopulatorThread -- start\n");

    // this thread will exit after some time
    // when then later some other thread tries to get keys, it will be much faster
    _this->cachePopulatorThread = std::thread([_this, t]() {
        // RenameThread("bitgreen-q-cachepop");
        for (size_t i = 0; i < _this->members.size() && !_this->stopCachePopulatorThread && !ShutdownRequested(); i++) {
            if (_this->qc.validMembers[i]) {
                _this->GetPubKeyShare(i);
            }
        }
        LogPrint(BCLog::LLMQ, "CQuorum::StartCachePopulatorThread -- done. time=%d\n", t.count());
    });
}

CQuorumManager::CQuorumManager(CSpecialDB& _specialDb, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    specialDb(_specialDb),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager)
{
}

void CQuorumManager::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
{
    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }

    for (auto& p : Params().GetConsensus().llmqs) {
        EnsureQuorumConnections(p.first, pindexNew);
    }
}

void CQuorumManager::EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex* pindexNew)
{
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto myProTxHash = activeMasternodeInfo.proTxHash;
    auto lastQuorums = ScanQuorums(llmqType, pindexNew, (size_t)params.keepOldConnections);

    auto connmanQuorumsToDelete = g_connman->GetMasternodeQuorums(llmqType);

    // don't remove connections for the currently in-progress DKG round
    int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % params.dkgInterval);
    auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
    connmanQuorumsToDelete.erase(curDkgBlock);

    for (auto& quorum : lastQuorums) {
        if (!quorum->IsMember(myProTxHash) && !gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS)) {
            continue;
        }

        if (!g_connman->HasMasternodeQuorumNodes(llmqType, quorum->qc.quorumHash)) {
            std::set<uint256> connections;
            if (quorum->IsMember(myProTxHash)) {
                connections = CLLMQUtils::GetQuorumConnections(llmqType, quorum->pindexQuorum, myProTxHash);
            } else {
                auto cindexes = CLLMQUtils::CalcDeterministicWatchConnections(llmqType, quorum->pindexQuorum, quorum->members.size(), 1);
                for (auto idx : cindexes) {
                    connections.emplace(quorum->members[idx]->proTxHash);
                }
            }
            if (!connections.empty()) {
                if (LogAcceptCategory(BCLog::LLMQ)) {
                    auto mnList = deterministicMNManager->GetListAtChainTip();
                    std::string debugMsg = strprintf("CQuorumManager::%s -- adding masternodes quorum connections for quorum %s:\n", __func__, quorum->qc.quorumHash.ToString());
                    for (auto& c : connections) {
                        auto dmn = mnList.GetValidMN(c);
                        if (!dmn) {
                            debugMsg += strprintf("  %s (not in valid MN set anymore)\n", c.ToString());
                        } else {
                            debugMsg += strprintf("  %s (%s)\n", c.ToString(), dmn->pdmnState->addr.ToString());
                        }
                    }
                    LogPrint(BCLog::LLMQ, debugMsg.c_str());
                }
                g_connman->AddMasternodeQuorumNodes(llmqType, quorum->qc.quorumHash, connections);
            }
        }
        connmanQuorumsToDelete.erase(quorum->qc.quorumHash);
    }

    for (auto& qh : connmanQuorumsToDelete) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, qh.ToString());
        g_connman->RemoveMasternodeQuorumNodes(llmqType, qh);
    }
}

bool CQuorumManager::BuildQuorumFromCommitment(const CFinalCommitment& qc, const CBlockIndex* pindexQuorum, const uint256& minedBlockHash, std::shared_ptr<CQuorum>& quorum) const
{
    assert(pindexQuorum);
    assert(qc.quorumHash == pindexQuorum->GetBlockHash());

    auto members = CLLMQUtils::GetAllQuorumMembers((Consensus::LLMQType)qc.llmqType, pindexQuorum);

    quorum->Init(qc, pindexQuorum, minedBlockHash, members);

    bool hasValidVvec = false;
    if (quorum->ReadContributions(specialDb)) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(qc, quorum)) {
            quorum->WriteContributions(specialDb);
            hasValidVvec = true;
        } else {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- quorum.ReadContributions and BuildQuorumContributions for block %s failed\n", __func__, qc.quorumHash.ToString());
        }
    }

    if (hasValidVvec) {
        // pre-populate caches in the background
        // recovering public key shares is quite expensive and would result in serious lags for the first few signing
        // sessions if the shares would be calculated on-demand
        CQuorum::StartCachePopulatorThread(quorum);
    }

    return true;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitment& fqc, std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions((Consensus::LLMQType)fqc.llmqType, quorum->pindexQuorum, fqc.validMembers, memberIndexes, vvecs, skContributions)) {
        return false;
    }

    BLSVerificationVectorPtr quorumVvec;
    CBLSSecretKey skShare;

    cxxtimer::Timer t2(true);
    quorumVvec = blsWorker.BuildQuorumVerificationVector(vvecs);
    if (quorumVvec == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build quorumVvec\n", __func__);
        // without the quorum vvec, there can't be a skShare, so we fail here. Failure is not fatal here, as it still
        // allows to use the quorum as a non-member (verification through the quorum pub key)
        return false;
    }
    skShare = blsWorker.AggregateSecretKeys(skContributions);
    if (!skShare.IsValid()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build skShare\n", __func__);
        // We don't bail out here as this is not a fatal error and still allows us to recover public key shares (as we
        // have a valid quorum vvec at this point)
    }
    t2.stop();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- built quorum vvec and skShare. time=%d\n", __func__, t2.count());

    quorum->quorumVvec = quorumVvec;
    quorum->skShare = skShare;

    return true;
}

bool CQuorumManager::HasQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    return quorumBlockProcessor->HasMinedCommitment(llmqType, quorumHash);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, size_t maxCount)
{
    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = ChainActive().Tip();
    }
    return ScanQuorums(llmqType, pindex, maxCount);
}

std::vector<CQuorumCPtr> CQuorumManager::ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t maxCount)
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto cacheKey = std::make_pair(llmqType, pindexStart->GetBlockHash());
    const size_t cacheMaxSize = params.signingActiveQuorumCount + 1;

    std::vector<CQuorumCPtr> result;

    if (maxCount <= cacheMaxSize) {
        LOCK(quorumsCacheCs);
        if (scanQuorumsCache.get(cacheKey, result)) {
            if (result.size() > maxCount) {
                result.resize(maxCount);
            }
            return result;
        }
    }

    bool storeCache = false;
    size_t maxCount2 = maxCount;
    if (maxCount2 <= cacheMaxSize) {
        maxCount2 = cacheMaxSize;
        storeCache = true;
    }

    auto quorumIndexes = quorumBlockProcessor->GetMinedCommitmentsUntilBlock(params.type, pindexStart, maxCount2);
    result.reserve(quorumIndexes.size());

    for (auto& quorumIndex : quorumIndexes) {
        assert(quorumIndex);
        auto quorum = GetQuorum(params.type, quorumIndex);
        assert(quorum != nullptr);
        result.emplace_back(quorum);
    }

    if (storeCache) {
        LOCK(quorumsCacheCs);
        scanQuorumsCache.insert(cacheKey, result);
    }

    if (result.size() > maxCount) {
        result.resize(maxCount);
    }

    return result;
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    CBlockIndex* pindexQuorum;
    {
        LOCK(cs_main);
        auto quorumIt = ::BlockIndex().find(quorumHash);

        if (quorumIt == ::BlockIndex().end()) {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found", __func__, quorumHash.ToString());
            return nullptr;
        }
        pindexQuorum = quorumIt->second;
    }
    return GetQuorum(llmqType, pindexQuorum);
}

CQuorumCPtr CQuorumManager::GetQuorum(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum)
{
    assert(pindexQuorum);

    auto quorumHash = pindexQuorum->GetBlockHash();

    // we must check this before we look into the cache. Reorgs might have happened which would mean we might have
    // cached quorums which are not in the active chain anymore
    if (!HasQuorum(llmqType, quorumHash)) {
        return nullptr;
    }

    LOCK(quorumsCacheCs);

    auto it = quorumsCache.find(std::make_pair(llmqType, quorumHash));
    if (it != quorumsCache.end()) {
        return it->second;
    }

    CFinalCommitment qc;
    uint256 minedBlockHash;
    if (!quorumBlockProcessor->GetMinedCommitment(llmqType, quorumHash, qc, minedBlockHash)) {
        return nullptr;
    }

    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto quorum = std::make_shared<CQuorum>(params, blsWorker);

    if (!BuildQuorumFromCommitment(qc, pindexQuorum, minedBlockHash, quorum)) {
        return nullptr;
    }

    quorumsCache.emplace(std::make_pair(llmqType, quorumHash), quorum);

    return quorum;
}

CQuorumCPtr CQuorumManager::GetNewestQuorum(Consensus::LLMQType llmqType)
{
    auto quorums = ScanQuorums(llmqType, 1);
    if (quorums.empty()) {
        return nullptr;
    }
    return quorums.front();
}

}
