// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_dkgsession.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_init.h>
#include <llmq/quorums_utils.h>

#include <evo/specialtx.h>

#include <masternode/activemasternode.h>
#include <chainparams.h>
#include <init.h>
#include <masternode/masternodesync.h>
#include <univalue.h>
#include <validation.h>
#include <shutdown.h>
#include <cxxtimer.hpp>
#include <net.h>
#include <netmessagemaker.h>
#include <net_processing.h>
namespace llmq
{

static const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
static const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

CQuorumManager* quorumManager;

RecursiveMutex cs_data_requests;
static std::unordered_map<std::pair<uint256, bool>, CQuorumDataRequest, StaticSaltedHasher> mapQuorumDataRequests;

static uint256 MakeQuorumKey(const CQuorum& q)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << q.params.type;
    hw << q.qc.quorumHash;
    for (const auto& dmn : q.members) {
        hw << dmn->proTxHash;
    }
    return hw.GetHash();
}

CQuorum::CQuorum(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker) : params(_params), blsCache(_blsWorker), stopQuorumThreads(false)
{
    interruptQuorumDataReceived.reset();
}

CQuorum::~CQuorum()
{
    interruptQuorumDataReceived();
    stopQuorumThreads = true;
}

void CQuorum::Init(const CFinalCommitment& _qc, const CBlockIndex* _pindexQuorum, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members)
{
    qc = _qc;
    pindexQuorum = _pindexQuorum;
    members = _members;
    minedBlockHash = _minedBlockHash;
}

bool CQuorum::SetVerificationVector(const BLSVerificationVector& quorumVecIn)
{
    if (::SerializeHash(quorumVecIn) != qc.quorumVvecHash) {
        return false;
    }
    quorumVvec = std::make_shared<BLSVerificationVector>(quorumVecIn);
    return true;
}

bool CQuorum::SetSecretKeyShare(const CBLSSecretKey& secretKeyShare)
{
    if (!secretKeyShare.IsValid() || (secretKeyShare.GetPublicKey() != GetPubKeyShare(GetMemberIndex(activeMasternodeInfo.proTxHash)))) {
        return false;
    }
    skShare = secretKeyShare;
    return true;
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

const CBLSSecretKey &CQuorum::GetSkShare() const
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

void CQuorum::WriteContributions(CEvoDB& evoDb)
{
    uint256 dbKey = MakeQuorumKey(*this);

    if (quorumVvec != nullptr) {
        evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), *quorumVvec);
    }
    if (skShare.IsValid()) {
        evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);
    }
}

bool CQuorum::ReadContributions(CEvoDB& evoDb)
{
    uint256 dbKey = MakeQuorumKey(*this);

    BLSVerificationVector qv;
    if (evoDb.Read(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), qv)) {
        quorumVvec = std::make_shared<BLSVerificationVector>(std::move(qv));
    } else {
        return false;
    }

    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    evoDb.Read(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);

    return true;
}

void CQuorum::StartCachePopulatorThread(std::shared_ptr<CQuorum> _this)
{
    if (_this->quorumVvec == nullptr) {
        return;
    }
    if (_this->cachePopulatorThread.joinable()) {
        LogPrint(BCLog::LLMQ, "CQuorum::StartCachePopulatorThread -- thread already active\n");
        return;
    }
    LogPrint(BCLog::LLMQ, "CQuorum::StartCachePopulatorThread -- start\n");
    // this thread will exit after some time
    // when then later some other thread tries to get keys, it will be much faster
    std::weak_ptr<const CQuorum> weak_this(_this);
    _this->cachePopulatorThread = std::thread(&TraceThread<std::function<void()>>, "syscoin-q-cachepop", [weak_this] {
        auto objThis = weak_this.lock();
        if (objThis) {
            for (size_t i = 0; i < objThis->members.size() && !objThis->stopQuorumThreads && !ShutdownRequested(); i++) {
                if (objThis->qc.validMembers[i]) {
                    objThis->GetPubKeyShare(i);
                }
            }
        }
    });
    _this->cachePopulatorThread.detach();
}

size_t CQuorum::GetQuorumRecoveryStartOffset(const CBlockIndex* pIndex) const
{
    const size_t nActiveQuorums = params.signingActiveQuorumCount + 1;
    std::vector<CQuorumCPtr> vecQuorums;
    quorumManager->ScanQuorums(qc.llmqType, pIndex, nActiveQuorums, vecQuorums);
    assert(vecQuorums.size() > 0);
    std::set<uint256> setAllTypeMembers;
    for (auto& pQuorum : vecQuorums) {
        auto& vecValid = pQuorum->qc.validMembers;
        for (size_t i = 0; i< vecValid.size(); ++i) {
            if (vecValid[i]) {
                setAllTypeMembers.emplace(pQuorum->members[i]->proTxHash);
            }
        }
    }
    std::vector<uint256> vecAllTypeMembers{setAllTypeMembers.begin(), setAllTypeMembers.end()};
    std::sort(vecAllTypeMembers.begin(), vecAllTypeMembers.end());
    size_t nMyIndex{0};
    for (size_t i = 0; i< vecAllTypeMembers.size(); ++i) {
        if (activeMasternodeInfo.proTxHash == vecAllTypeMembers[i]) {
            nMyIndex = i;
            break;
        }
    }
    return nMyIndex % qc.validMembers.size();
}

void CQuorum::StartQuorumDataRecoveryThread(CConnman &connman, const CQuorumCPtr _this, const CBlockIndex* pIndex, uint16_t nDataMaskIn)
{
    if (_this->fQuorumDataRecoveryThreadRunning) {
        LogPrint(BCLog::LLMQ, "CQuorum::%s -- Already running\n", __func__);
        return;
    }
    _this->fQuorumDataRecoveryThreadRunning = true;
    std::weak_ptr<const CQuorum> weak_this(_this);
    std::thread(&TraceThread<std::function<void()>>, "syscoin-q-recovery", [&connman, weak_this, pIndex, nDataMaskIn] {
        auto objThis = weak_this.lock();
        if (objThis) {
            size_t nTries{0};
            uint16_t nDataMask{nDataMaskIn};
            int64_t nTimeLastSuccess{0};
            uint256* pCurrentMemberHash{nullptr};
            std::vector<uint256> vecMemberHashes;
            const size_t nMyStartOffset{objThis->GetQuorumRecoveryStartOffset(pIndex)};
            const int64_t nRequestTimeout{10};

            auto printLog = [&](const std::string& strMessage) {
                const std::string strMember{pCurrentMemberHash == nullptr ? "nullptr" : pCurrentMemberHash->ToString()};
                LogPrint(BCLog::LLMQ, "%s -- %s - for llmqType %d, quorumHash %s, nDataMask (%d/%d), pCurrentMemberHash %s, nTries %d\n",
                    "syscoin-q-recovery", strMessage, objThis->qc.llmqType, objThis->qc.quorumHash.ToString(), nDataMask, nDataMaskIn, strMember, nTries);
            };
            printLog("Start");

            while (!masternodeSync.IsBlockchainSynced() && !objThis->stopQuorumThreads && !ShutdownRequested()) {
                objThis->interruptQuorumDataReceived.reset();
                objThis->interruptQuorumDataReceived.sleep_for(std::chrono::seconds(nRequestTimeout));
            }

            if (objThis->stopQuorumThreads || ShutdownRequested()) {
                printLog("Aborted");
                return;
            }

            vecMemberHashes.reserve(objThis->qc.validMembers.size());
            for (auto& member : objThis->members) {
                if (objThis->IsValidMember(member->proTxHash) && member->proTxHash != activeMasternodeInfo.proTxHash) {
                    vecMemberHashes.push_back(member->proTxHash);
                }
            }
            std::sort(vecMemberHashes.begin(), vecMemberHashes.end());

            printLog("Try to request");

            while (nDataMask > 0 && !objThis->stopQuorumThreads && !ShutdownRequested()) {

                if (nDataMask & llmq::CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR && objThis->quorumVvec != nullptr) {
                    nDataMask &= ~llmq::CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR;
                    printLog("Received quorumVvec");
                }

                if (nDataMask & llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS && objThis->skShare.IsValid()) {
                    nDataMask &= ~llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS;
                    printLog("Received skShare");
                }

                if (nDataMask == 0) {
                    printLog("Success");
                    break;
                }

                if ((GetAdjustedTime() - nTimeLastSuccess) > nRequestTimeout) {
                    if (nTries >= vecMemberHashes.size()) {
                        printLog("All tried but failed");
                        break;
                    }
                    // Access the member list of the quorum with the calculated offset applied to balance the load equally
                    pCurrentMemberHash = &vecMemberHashes[(nMyStartOffset + nTries++) % vecMemberHashes.size()];
                    {
                        LOCK(cs_data_requests);
                        auto it = mapQuorumDataRequests.find(std::make_pair(*pCurrentMemberHash, true));
                        if (it != mapQuorumDataRequests.end() && !it->second.IsExpired()) {
                            printLog("Already asked");
                            continue;
                        }
                    }
                    // Sleep a bit depending on the start offset to balance out multiple requests to same masternode
                    objThis->interruptQuorumDataReceived.sleep_for(std::chrono::milliseconds(nMyStartOffset * 100));
                    nTimeLastSuccess = GetAdjustedTime();
                    connman.AddPendingMasternode(*pCurrentMemberHash);
                    printLog("Connect");
                }

                connman.ForEachNode([&](CNode* pNode) {

                    if (pCurrentMemberHash == nullptr || pNode->verifiedProRegTxHash != *pCurrentMemberHash) {
                        return;
                    }

                    if (quorumManager->RequestQuorumData(pNode, objThis->qc.llmqType, objThis->pindexQuorum, nDataMask, activeMasternodeInfo.proTxHash)) {
                        nTimeLastSuccess = GetAdjustedTime();
                        printLog("Requested");
                    } else {
                        LOCK(cs_data_requests);
                        auto it = mapQuorumDataRequests.find(std::make_pair(pNode->verifiedProRegTxHash, true));
                        if (it == mapQuorumDataRequests.end()) {
                            printLog("Failed");
                            pNode->fDisconnect = true;
                            pCurrentMemberHash = nullptr;
                            return;
                        } else if (it->second.IsProcessed()) {
                            printLog("Processed");
                            pNode->fDisconnect = true;
                            pCurrentMemberHash = nullptr;
                            return;
                        } else {
                            printLog("Waiting");
                            return;
                        }
                    }
                });
                objThis->interruptQuorumDataReceived.reset();
                objThis->interruptQuorumDataReceived.sleep_for(std::chrono::seconds(1));
            }
            objThis->fQuorumDataRecoveryThreadRunning = false;
            printLog("Done");
        }
    }).detach();
}

CQuorumManager::CQuorumManager(CEvoDB& _evoDb, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    evoDb(_evoDb),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager)
{
    CLLMQUtils::InitQuorumsCache(mapQuorumsCache);
    CLLMQUtils::InitQuorumsCache(scanQuorumsCache);
}

void CQuorumManager::TriggerQuorumDataRecoveryThreads(const CBlockIndex* pIndex) const
{
    if (!fMasternodeMode || !CLLMQUtils::QuorumDataRecoveryEnabled() || pIndex == nullptr) {
        return;
    }

    const std::set<uint8_t> setQuorumVvecSyncTypes = CLLMQUtils::GetEnabledQuorumVvecSyncTypes();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Process block %s\n", __func__, pIndex->GetBlockHash().ToString());

    for (auto& llmq : Params().GetConsensus().llmqs) {
        // Process signingActiveQuorumCount + 1 quorums for all available llmqTypes
        std::vector<CQuorumCPtr> vecQuorums;
        ScanQuorums(llmq.first, pIndex, llmq.second.signingActiveQuorumCount + 1, vecQuorums);

        // First check if we are member of any quorum of this type
        bool fWeAreQuorumTypeMember{false};
        for (const auto& pQuorum : vecQuorums) {
            if (pQuorum->IsValidMember(activeMasternodeInfo.proTxHash)) {
                fWeAreQuorumTypeMember = true;
                break;
            }
        }

        for (const auto& pQuorum : vecQuorums) {
            // If there is already a thread running for this specific quorum skip it
            if (pQuorum->fQuorumDataRecoveryThreadRunning) {
                continue;
            }

            uint16_t nDataMask{0};
            const bool fWeAreQuorumMember = pQuorum->IsValidMember(activeMasternodeInfo.proTxHash);
            const bool fTypeRequestsEnabled = setQuorumVvecSyncTypes.count(pQuorum->qc.llmqType) > 0;

            if ((fWeAreQuorumMember || (fWeAreQuorumTypeMember && fTypeRequestsEnabled)) && pQuorum->quorumVvec == nullptr) {
                nDataMask |= llmq::CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR;
            }

            if (fWeAreQuorumMember && !pQuorum->skShare.IsValid()) {
                nDataMask |= llmq::CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS;
            }

            if (nDataMask == 0) {
                LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- No data needed from (%d, %s) at height %d\n",
                    __func__, pQuorum->qc.llmqType, pQuorum->qc.quorumHash.ToString(), pIndex->nHeight);
                continue;
            }

            // Finally start the thread which triggers the requests for this quorum
            CQuorum::StartQuorumDataRecoveryThread(dkgManager.connman, pQuorum, pIndex, nDataMask);
        }
    }
}

void CQuorumManager::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
{
    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }

    for (auto& p : Params().GetConsensus().llmqs) {
        EnsureQuorumConnections(p.first, pindexNew);
    }

    // Cleanup expired data requests
    LOCK(cs_data_requests);
    auto it = mapQuorumDataRequests.begin();
    while (it != mapQuorumDataRequests.end()) {
        if (it->second.IsExpired()) {
            it = mapQuorumDataRequests.erase(it);
        } else {
            ++it;
        }
    }

    TriggerQuorumDataRecoveryThreads(pindexNew);
}

void CQuorumManager::EnsureQuorumConnections(uint8_t llmqType, const CBlockIndex* pindexNew)
{
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);
    auto myProTxHash = activeMasternodeInfo.proTxHash;
    std::vector<CQuorumCPtr> lastQuorums;
    ScanQuorums(llmqType, pindexNew, (size_t)params.keepOldConnections, lastQuorums);
    std::set<uint256> connmanQuorumsToDelete;
    dkgManager.connman.GetMasternodeQuorums(llmqType, connmanQuorumsToDelete);
    // don't remove connections for the currently in-progress DKG round
    int curDkgHeight = pindexNew->nHeight - (pindexNew->nHeight % params.dkgInterval);
    auto curDkgBlock = pindexNew->GetAncestor(curDkgHeight)->GetBlockHash();
    connmanQuorumsToDelete.erase(curDkgBlock);
    bool allowWatch = gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS);
    for (auto& quorum : lastQuorums) {
        if (!quorum->IsMember(myProTxHash) && !allowWatch) {
            continue;
        }
        CLLMQUtils::EnsureQuorumConnections(llmqType, quorum->pindexQuorum, myProTxHash, allowWatch, dkgManager.connman);
        connmanQuorumsToDelete.erase(quorum->qc.quorumHash);
    }
    for (auto& qh : connmanQuorumsToDelete) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- removing masternodes quorum connections for quorum %s:\n", __func__, qh.ToString());
        dkgManager.connman.RemoveMasternodeQuorumNodes(llmqType, qh);
    }
}

CQuorumPtr CQuorumManager::BuildQuorumFromCommitment(const uint8_t llmqType, const CBlockIndex* pindexQuorum) const
{
    AssertLockHeld(quorumsCacheCs);
    assert(pindexQuorum);

    CFinalCommitment qc;
    const uint256& quorumHash{pindexQuorum->GetBlockHash()};
    uint256 minedBlockHash;
    if (!quorumBlockProcessor->GetMinedCommitment(llmqType, quorumHash, qc, minedBlockHash)) {
        return nullptr;
    }
    assert(qc.quorumHash == pindexQuorum->GetBlockHash());

    auto quorum = std::make_shared<CQuorum>(Params().GetConsensus().llmqs.at(llmqType), blsWorker);
    std::vector<CDeterministicMNCPtr> members;
    CLLMQUtils::GetAllQuorumMembers(qc.llmqType, pindexQuorum, members);

    quorum->Init(qc, pindexQuorum, minedBlockHash, members);

    bool hasValidVvec = false;
    if (quorum->ReadContributions(evoDb)) {
        hasValidVvec = true;
    } else {
        if (BuildQuorumContributions(qc, quorum)) {
            quorum->WriteContributions(evoDb);
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

    mapQuorumsCache[llmqType].insert(quorumHash, quorum);

    return quorum;
}

bool CQuorumManager::BuildQuorumContributions(const CFinalCommitment& fqc, std::shared_ptr<CQuorum>& quorum) const
{
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions(fqc.llmqType, quorum->pindexQuorum, fqc.validMembers, memberIndexes, vvecs, skContributions)) {
        return false;
    }


    cxxtimer::Timer t2(true);
    quorum->quorumVvec = blsWorker.BuildQuorumVerificationVector(vvecs);
    if (quorum->quorumVvec == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build quorumVvec\n", __func__);
        // without the quorum vvec, there can't be a skShare, so we fail here. Failure is not fatal here, as it still
        // allows to use the quorum as a non-member (verification through the quorum pub key)
        return false;
    }
	quorum->skShare = blsWorker.AggregateSecretKeys(skContributions);
    if (!quorum->skShare.IsValid()) {
        quorum->skShare.Reset();
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- failed to build skShare\n", __func__);
        // We don't bail out here as this is not a fatal error and still allows us to recover public key shares (as we
        // have a valid quorum vvec at this point)
    }
    t2.stop();

    LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- built quorum vvec and skShare. time=%d\n", __func__, t2.count());

    return true;
}

bool CQuorumManager::HasQuorum(uint8_t llmqType, const uint256& quorumHash)
{
    return quorumBlockProcessor->HasMinedCommitment(llmqType, quorumHash);
}

bool CQuorumManager::RequestQuorumData(CNode* pFrom, uint8_t llmqType, const CBlockIndex* pQuorumIndex, uint16_t nDataMask, const uint256& proTxHash)
{
    if (pFrom == nullptr || (pFrom->verifiedProRegTxHash.IsNull() && !pFrom->qwatch)) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- pFrom is neither a verified masternode nor a qwatch connection\n", __func__);
        return false;
    }
    if (Params().GetConsensus().llmqs.count(llmqType) == 0) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid llmqType: %d\n", __func__, llmqType);
        return false;
    }
    if (pQuorumIndex == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Invalid pQuorumIndex: nullptr\n", __func__);
        return false;
    }
    if (GetQuorum(llmqType, pQuorumIndex) == nullptr) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Quorum not found: %s, %d\n", __func__, pQuorumIndex->GetBlockHash().ToString(), llmqType);
        return false;
    }

    LOCK(cs_data_requests);
    auto key = std::make_pair(pFrom->verifiedProRegTxHash, true);
    auto it = mapQuorumDataRequests.emplace(key, CQuorumDataRequest(llmqType, pQuorumIndex->GetBlockHash(), nDataMask, proTxHash));
    if (!it.second && !it.first->second.IsExpired()) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- Already requested\n", __func__);
        return false;
    }

    CNetMsgMaker msgMaker(pFrom->GetCommonVersion());
    dkgManager.connman.PushMessage(pFrom, msgMaker.Make(NetMsgType::QGETDATA, it.first->second));

    return true;
}

void CQuorumManager::ScanQuorums(uint8_t llmqType, size_t nCountRequested, std::vector<CQuorumCPtr>& quorums) const
{
    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = ::ChainActive().Tip();
    }
    ScanQuorums(llmqType, pindex, nCountRequested, quorums);
}

void CQuorumManager::ScanQuorums(uint8_t llmqType, const CBlockIndex* pindexStart, size_t nCountRequested, std::vector<CQuorumCPtr>& vecResultQuorums) const
{
    if (pindexStart == nullptr || nCountRequested == 0) {
        return;
    }

    bool fCacheExists{false};
    void* pIndexScanCommitments{(void*)pindexStart};
    size_t nScanCommitments{nCountRequested};

    {
        LOCK(quorumsCacheCs);
        auto& cache = scanQuorumsCache[llmqType];
        fCacheExists = cache.get(pindexStart->GetBlockHash(), vecResultQuorums);
        if (fCacheExists) {
            // We have exactly what requested so just return it
            if (vecResultQuorums.size() == nCountRequested) {
                return;
            }
            // If we have more cached than requested return only a subvector
            if (vecResultQuorums.size() > nCountRequested) {
                vecResultQuorums.resize(nCountRequested);
                return;
            }
            // If we have cached quorums but not enough, subtract what we have from the count and the set correct index where to start
            // scanning for the rests
            if(vecResultQuorums.size() > 0) {
                nScanCommitments -= vecResultQuorums.size();
                pIndexScanCommitments = (void*)vecResultQuorums.back()->pindexQuorum->pprev;
            }
        } else {
            // If there is nothing in cache request at least cache.max_size() because this gets cached then later
            nScanCommitments = std::max(nCountRequested, cache.max_size());
        }
    }
    // Get the block indexes of the mined commitments to build the required quorums from
    std::vector<const CBlockIndex*> quorumIndexes;
    quorumBlockProcessor->GetMinedCommitmentsUntilBlock(llmqType, (const CBlockIndex*)pIndexScanCommitments, nScanCommitments, quorumIndexes);
    vecResultQuorums.reserve(vecResultQuorums.size() + quorumIndexes.size());

    for (auto& quorumIndex : quorumIndexes) {
        assert(quorumIndex);
        auto quorum = GetQuorum(llmqType, quorumIndex);
        if(quorum)
            vecResultQuorums.emplace_back(quorum);
    }

    size_t nCountResult{vecResultQuorums.size()};
    if (nCountResult > 0 && !fCacheExists) {
        LOCK(quorumsCacheCs);
        // Don't cache more than cache.max_size() elements
        auto& cache = scanQuorumsCache[llmqType];
        size_t nCacheEndIndex = std::min(nCountResult, cache.max_size());
        cache.emplace(pindexStart->GetBlockHash(), {vecResultQuorums.begin(), vecResultQuorums.begin() + nCacheEndIndex});
    }
    // Don't return more than nCountRequested elements
    size_t nResultEndIndex = std::min(nCountResult, nCountRequested);
    vecResultQuorums.resize(nResultEndIndex);
}

CQuorumCPtr CQuorumManager::GetQuorum(uint8_t llmqType, const uint256& quorumHash) const
{
    CBlockIndex* pindexQuorum;
    {
        LOCK(cs_main);

        pindexQuorum = g_chainman.m_blockman.LookupBlockIndex(quorumHash);
        if (!pindexQuorum) {
            LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- block %s not found\n", __func__, quorumHash.ToString());
            return nullptr;
        }
    }
    return GetQuorum(llmqType, pindexQuorum);
}


CQuorumCPtr CQuorumManager::GetQuorum(uint8_t llmqType, const CBlockIndex* pindexQuorum) const
{
    assert(pindexQuorum);

    auto quorumHash = pindexQuorum->GetBlockHash();

    // we must check this before we look into the cache. Reorgs might have happened which would mean we might have
    // cached quorums which are not in the active chain anymore
    if (!HasQuorum(llmqType, quorumHash)) {
        return nullptr;
    }

    LOCK(quorumsCacheCs);
    CQuorumPtr pQuorum;
    if (mapQuorumsCache[llmqType].get(quorumHash, pQuorum)) {
        return pQuorum;
    }

    return BuildQuorumFromCommitment(llmqType, pindexQuorum);
}

void CQuorumManager::ProcessMessage(CNode* pFrom, const std::string& strCommand, CDataStream& vRecv)
{
    auto strFunc = __func__;
    auto errorHandler = [&](const std::string strError, int nScore = 10) {
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s: %s, from peer=%d\n", strFunc, strCommand, strError, pFrom->GetId());
        if (nScore > 0) {
            dkgManager.peerman.Misbehaving(pFrom->GetId(), nScore, strError);
        }
    };

    if (strCommand == NetMsgType::QGETDATA) {

        if (!fMasternodeMode || pFrom == nullptr || (pFrom->verifiedProRegTxHash.IsNull() && !pFrom->qwatch)) {
            errorHandler("Not a verified masternode or a qwatch connection");
            return;
        }

        CQuorumDataRequest request;
        vRecv >> request;

        auto sendQDATA = [&](CQuorumDataRequest::Errors nError = CQuorumDataRequest::Errors::UNDEFINED,
                             const CDataStream& body = CDataStream(SER_NETWORK, PROTOCOL_VERSION)) {
            request.SetError(nError);
            CDataStream ssResponse(SER_NETWORK, pFrom->GetCommonVersion(), request, body);
            dkgManager.connman.PushMessage(pFrom, CNetMsgMaker(pFrom->GetCommonVersion()).Make(NetMsgType::QDATA, ssResponse));
        };

        {
            LOCK2(cs_main, cs_data_requests);
            auto key = std::make_pair(pFrom->verifiedProRegTxHash, false);
            auto it = mapQuorumDataRequests.find(key);
            if (it == mapQuorumDataRequests.end()) {
                it = mapQuorumDataRequests.emplace(key, request).first;
            } else if(it->second.IsExpired()) {
                it->second = request;
            } else {
                errorHandler("Request limit exceeded", 25);
            }
        }

        if (Params().GetConsensus().llmqs.count(request.GetLLMQType()) == 0) {
            sendQDATA(CQuorumDataRequest::Errors::QUORUM_TYPE_INVALID);
            return;
        }

        const CBlockIndex* pQuorumIndex{nullptr};
        {
            LOCK(cs_main);
            pQuorumIndex = g_chainman.m_blockman.LookupBlockIndex(request.GetQuorumHash());
        }
        if (pQuorumIndex == nullptr) {
            sendQDATA(CQuorumDataRequest::Errors::QUORUM_BLOCK_NOT_FOUND);
            return;
        }

        const CQuorumCPtr pQuorum = GetQuorum(request.GetLLMQType(), pQuorumIndex);
        if (pQuorum == nullptr) {
            sendQDATA(CQuorumDataRequest::Errors::QUORUM_NOT_FOUND);
            return;
        }

        CDataStream ssResponseData(SER_NETWORK, pFrom->GetCommonVersion());

        // Check if request wants QUORUM_VERIFICATION_VECTOR data
        if (request.GetDataMask() & CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR) {

            if (!pQuorum->quorumVvec) {
                sendQDATA(CQuorumDataRequest::Errors::QUORUM_VERIFICATION_VECTOR_MISSING);
                return;
            }

            ssResponseData << *pQuorum->quorumVvec;
        }

        // Check if request wants ENCRYPTED_CONTRIBUTIONS data
        if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {

            int memberIdx = pQuorum->GetMemberIndex(request.GetProTxHash());
            if (memberIdx == -1) {
                sendQDATA(CQuorumDataRequest::Errors::MASTERNODE_IS_NO_MEMBER);
                return;
            }

            std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
            if (!quorumDKGSessionManager->GetEncryptedContributions(request.GetLLMQType(), pQuorumIndex, pQuorum->qc.validMembers, request.GetProTxHash(), vecEncrypted)) {
                sendQDATA(CQuorumDataRequest::Errors::ENCRYPTED_CONTRIBUTIONS_MISSING);
                return;
            }

            ssResponseData << vecEncrypted;
        }

        sendQDATA(CQuorumDataRequest::Errors::NONE, ssResponseData);
        return;
    }

    if (strCommand == NetMsgType::QDATA) {

        bool fIsWatching = gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS);
        if ((!fMasternodeMode && !fIsWatching) || pFrom == nullptr || (pFrom->verifiedProRegTxHash.IsNull() && !pFrom->qwatch)) {
            errorHandler("Not a verified masternode or a qwatch connection");
            return;
        }

        CQuorumDataRequest request;
        vRecv >> request;

        {
            LOCK2(cs_main, cs_data_requests);
            auto it = mapQuorumDataRequests.find(std::make_pair(pFrom->verifiedProRegTxHash, true));
            if (it == mapQuorumDataRequests.end()) {
                errorHandler("Not requested");
                return;
            }
            if (it->second.IsProcessed()) {
                errorHandler("Already received");
                return;
            }
            if (request != it->second) {
                errorHandler("Not like requested");
                return;
            }
            it->second.SetProcessed();
        }

        if (request.GetError() != CQuorumDataRequest::Errors::NONE) {
            errorHandler(strprintf("Error %d", request.GetError()), 0);
            return;
        }

        CQuorumPtr pQuorum;
        {
            LOCK(quorumsCacheCs);
            if (!mapQuorumsCache[request.GetLLMQType()].get(request.GetQuorumHash(), pQuorum)) {
                errorHandler("Quorum not found", 0); // Don't bump score because we asked for it
                return;
            }
        }

        // Check if request has QUORUM_VERIFICATION_VECTOR data
        if (request.GetDataMask() & CQuorumDataRequest::QUORUM_VERIFICATION_VECTOR) {

            BLSVerificationVector verficationVector;
            vRecv >> verficationVector;

            if (pQuorum->SetVerificationVector(verficationVector)) {
                CQuorum::StartCachePopulatorThread(pQuorum);
            } else {
                errorHandler("Invalid quorum verification vector");
                return;
            }
        }

        // Check if request has ENCRYPTED_CONTRIBUTIONS data
        if (request.GetDataMask() & CQuorumDataRequest::ENCRYPTED_CONTRIBUTIONS) {

            if (pQuorum->quorumVvec->size() != (size_t)pQuorum->params.threshold) {
                errorHandler("No valid quorum verification vector available", 0); // Don't bump score because we asked for it
                return;
            }

            int memberIdx = pQuorum->GetMemberIndex(request.GetProTxHash());
            if (memberIdx == -1) {
                errorHandler("Not a member of the quorum", 0); // Don't bump score because we asked for it
                return;
            }

            std::vector<CBLSIESEncryptedObject<CBLSSecretKey>> vecEncrypted;
            vRecv >> vecEncrypted;

            BLSSecretKeyVector vecSecretKeys;
            vecSecretKeys.resize(vecEncrypted.size());
            for (size_t i = 0; i < vecEncrypted.size(); ++i) {
                if (!vecEncrypted[i].Decrypt(memberIdx, *activeMasternodeInfo.blsKeyOperator, vecSecretKeys[i], PROTOCOL_VERSION)) {
                    errorHandler("Failed to decrypt");
                    return;
                }
            }

            CBLSSecretKey secretKeyShare = blsWorker.AggregateSecretKeys(vecSecretKeys);
            if (!pQuorum->SetSecretKeyShare(secretKeyShare)) {
                errorHandler("Invalid secret key share received");
                return;
            }
        }
        pQuorum->WriteContributions(evoDb);
        pQuorum->interruptQuorumDataReceived.reset();
        return;
    }
}

} // namespace llmq
