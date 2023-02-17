// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_utils.h>

#include <evo/deterministicmns.h>

#include <chainparams.h>
#include <net_processing.h>
#include <spork.h>
#include <validation.h>
#include <util/system.h>
namespace llmq
{

CDKGSessionManager* quorumDKGSessionManager;

static const std::string DB_VVEC = "qdkg_V";
static const std::string DB_SKCONTRIB = "qdkg_S";
static const std::string DB_ENC_CONTRIB = "qdkg_E";

CDKGSessionManager::CDKGSessionManager(CBLSWorker& blsWorker, CConnman &_connman, PeerManager& _peerman, ChainstateManager& _chainman, bool unitTests, bool fWipe) :
    connman(_connman),
    peerman(_peerman)
{
    db = std::make_unique<CDBWrapper>(DBParams{
        .path = gArgs.GetDataDirNet() / "llmq/dkgdb",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = unitTests,
        .wipe_data = fWipe});
    MigrateDKG();
    if(Params().GetConsensus().llmqs.empty()) {
        return;
    }
    const auto &llmq = GetLLMQParams(fRegTest? Consensus::LLMQ_TEST: Consensus::LLMQ_400_60);
    dkgSessionHandlers.emplace(std::piecewise_construct,
            std::forward_as_tuple(llmq.type),
            std::forward_as_tuple(llmq, blsWorker, *this, peerman, _chainman));
    
}

void CDKGSessionManager::MigrateDKG()
{
    if (!db->IsEmpty()) return;

    LogPrint(BCLog::LLMQ, "CDKGSessionManager::%d -- start\n", __func__);

    CDBBatch batch(*db);
    auto oldDb = std::make_unique<CDBWrapper>(DBParams{
        .path = gArgs.GetDataDirNet() / "llmq",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = false,
        .wipe_data = false});
    std::unique_ptr<CDBIterator> pcursor(oldDb->NewIterator());

    auto start_vvec = std::make_tuple(DB_VVEC, (uint8_t)0, uint256(), uint256());
    pcursor->Seek(start_vvec);

    while (pcursor->Valid()) {
        decltype(start_vvec) k;
        BLSVerificationVector v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != DB_VVEC) {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_contrib = std::make_tuple(DB_SKCONTRIB, (uint8_t)0, uint256(), uint256());
    pcursor->Seek(start_contrib);

    while (pcursor->Valid()) {
        decltype(start_contrib) k;
        CBLSSecretKey v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != DB_SKCONTRIB) {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    auto start_enc_contrib = std::make_tuple(DB_ENC_CONTRIB, (uint8_t)0, uint256(), uint256());
    pcursor->Seek(start_enc_contrib);

    while (pcursor->Valid()) {
        decltype(start_enc_contrib) k;
        CBLSIESMultiRecipientObjects<CBLSSecretKey> v;

        if (!pcursor->GetKey(k) || std::get<0>(k) != DB_ENC_CONTRIB) {
            break;
        }
        if (!pcursor->GetValue(v)) {
            break;
        }

        batch.Write(k, v);

        if (batch.SizeEstimate() >= (1 << 24)) {
            db->WriteBatch(batch);
            batch.Clear();
        }

        pcursor->Next();
    }

    db->WriteBatch(batch);
    pcursor.reset();
    oldDb.reset();

    LogPrint(BCLog::LLMQ, "CDKGSessionManager::%d -- done\n", __func__);
}

void CDKGSessionManager::StartThreads()
{
    for (auto& it : dkgSessionHandlers) {
        it.second.StartThread();
    }
}

void CDKGSessionManager::StopThreads()
{
    for (auto& it : dkgSessionHandlers) {
        it.second.StopThread();
    }
}

void CDKGSessionManager::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
{
    CleanupCache();

    if (fInitialDownload)
        return;
    if (!deterministicMNManager || !deterministicMNManager->IsDIP3Enforced(pindexNew->nHeight))
        return;
    if (!IsQuorumDKGEnabled())
        return;

    for (auto& qt : dkgSessionHandlers) {
        qt.second.UpdatedBlockTip(pindexNew);
    }
}

void CDKGSessionManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (!IsQuorumDKGEnabled())
        return;

    if (strCommand != NetMsgType::QCONTRIB
        && strCommand != NetMsgType::QCOMPLAINT
        && strCommand != NetMsgType::QJUSTIFICATION
        && strCommand != NetMsgType::QPCOMMITMENT
        && strCommand != NetMsgType::QWATCH) {
        return;
    }

    if (strCommand == NetMsgType::QWATCH) {
        pfrom->qwatch = true;
        return;
    }
    PeerRef peer = peerman.GetPeerRef(pfrom->GetId());
    if (vRecv.empty()) {
        if(peer)
            peerman.Misbehaving(*peer, 100, "invalid recv size for DKG session");
        return;
    }

    // peek into the message and see which uint8_t it is. First byte of all messages is always the uint8_t
    uint8_t llmqType = static_cast<uint8_t>(*vRecv.begin());
    if (!dkgSessionHandlers.count(llmqType)) {
        if(peer)
            peerman.Misbehaving(*peer, 100, "DKG session invalid LLMQ type");
        return;
    }

    dkgSessionHandlers.at(llmqType).ProcessMessage(pfrom, strCommand, vRecv);
}

bool CDKGSessionManager::AlreadyHave(const uint256& hash) const
{
    if (!IsQuorumDKGEnabled())
        return false;
    
    for (const auto& p : dkgSessionHandlers) {
        auto& dkgType = p.second;
        LOCK(dkgType.cs);   
        if (dkgType.pendingContributions.HasSeen(hash)
            || dkgType.pendingComplaints.HasSeen(hash)
            || dkgType.pendingJustifications.HasSeen(hash)
            || dkgType.pendingPrematureCommitments.HasSeen(hash)) {
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetContribution(const uint256& hash, CDKGContribution& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;

    for (const auto& p : dkgSessionHandlers) {
        auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase_Initialized || dkgType.phase > QuorumPhase_Contribute) {
            continue;
        }
        LOCK(dkgType.curSession->invCs);
        auto it = dkgType.curSession->contributions.find(hash);
        if (it != dkgType.curSession->contributions.end()) {
            ret = it->second;
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetComplaint(const uint256& hash, CDKGComplaint& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;

    for (const auto& p : dkgSessionHandlers) {
        auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase_Contribute || dkgType.phase > QuorumPhase_Complain) {
            continue;
        }
        LOCK(dkgType.curSession->invCs);
        auto it = dkgType.curSession->complaints.find(hash);
        if (it != dkgType.curSession->complaints.end()) {
            ret = it->second;
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetJustification(const uint256& hash, CDKGJustification& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;

    for (const auto& p : dkgSessionHandlers) {
        auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase_Complain || dkgType.phase > QuorumPhase_Justify) {
            continue;
        }
        LOCK(dkgType.curSession->invCs);
        auto it = dkgType.curSession->justifications.find(hash);
        if (it != dkgType.curSession->justifications.end()) {
            ret = it->second;
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;

    for (const auto& p : dkgSessionHandlers) {
        auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase_Justify || dkgType.phase > QuorumPhase_Commit) {
            continue;
        }
        LOCK(dkgType.curSession->invCs);
        auto it = dkgType.curSession->prematureCommitments.find(hash);
        if (it != dkgType.curSession->prematureCommitments.end() && dkgType.curSession->validCommitments.count(hash)) {
            ret = it->second;
            return true;
        }
    }
    return false;
}

void CDKGSessionManager::WriteVerifiedVvecContribution(uint8_t llmqType, const uint256& hashQuorum, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec)
{
    db->Write(std::make_tuple(DB_VVEC, llmqType, hashQuorum, proTxHash), *vvec);
}

void CDKGSessionManager::WriteVerifiedSkContribution(uint8_t llmqType, const uint256& hashQuorum, const uint256& proTxHash, const CBLSSecretKey& skContribution)
{
    db->Write(std::make_tuple(DB_SKCONTRIB, llmqType, hashQuorum, proTxHash), skContribution);
}
void CDKGSessionManager::WriteEncryptedContributions(uint8_t llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSIESMultiRecipientObjects<CBLSSecretKey>& contributions)
{
    db->Write(std::make_tuple(DB_ENC_CONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), contributions);
}
bool CDKGSessionManager::GetVerifiedContributions(uint8_t llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, std::vector<uint16_t>& memberIndexesRet, std::vector<BLSVerificationVectorPtr>& vvecsRet, BLSSecretKeyVector& skContributionsRet) const
{
    LOCK(contributionsCacheCs);
    auto members = CLLMQUtils::GetAllQuorumMembers(GetLLMQParams(llmqType), pQuorumBaseBlockIndex);

    memberIndexesRet.clear();
    vvecsRet.clear();
    skContributionsRet.clear();
    memberIndexesRet.reserve(members.size());
    vvecsRet.reserve(members.size());
    skContributionsRet.reserve(members.size());
    for (size_t i = 0; i < members.size(); i++) {
        if (validMembers[i]) {
            const uint256& proTxHash = members[i]->proTxHash;
            ContributionsCacheKey cacheKey = {llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash};
            auto it = contributionsCache.find(cacheKey);
            if (it == contributionsCache.end()) {
                auto vvecPtr = std::make_shared<BLSVerificationVector>();
                CBLSSecretKey skContribution;
                if (!db->Read(std::make_tuple(DB_VVEC, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), *vvecPtr)) {
                    return false;
                }
                db->Read(std::make_tuple(DB_SKCONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), skContribution);

                it = contributionsCache.emplace(cacheKey, ContributionsCacheEntry{GetTimeMillis(), vvecPtr, skContribution}).first;
            }

            memberIndexesRet.emplace_back(i);
            vvecsRet.emplace_back(it->second.vvec);
            skContributionsRet.emplace_back(it->second.skContribution);
        }
    }
    return true;
}

bool CDKGSessionManager::GetEncryptedContributions(uint8_t llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, const uint256& nProTxHash, std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vecRet) const
{
    auto members = CLLMQUtils::GetAllQuorumMembers(GetLLMQParams(llmqType), pQuorumBaseBlockIndex);

    vecRet.clear();
    vecRet.reserve(members.size());

    size_t nRequestedMemberIdx{std::numeric_limits<size_t>::max()};
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i]->proTxHash == nProTxHash) {
            nRequestedMemberIdx = i;
            break;
        }
    }
    if (nRequestedMemberIdx == std::numeric_limits<size_t>::max()) {
        return false;
    }

    for (size_t i = 0; i < members.size(); i++) {
        if (validMembers[i]) {
            CBLSIESMultiRecipientObjects<CBLSSecretKey> encryptedContributions;
            if (!db->Read(std::make_tuple(DB_ENC_CONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), members[i]->proTxHash), encryptedContributions)) {
                return false;
            }
            vecRet.emplace_back(encryptedContributions.Get(nRequestedMemberIdx));
        }
    }
    return true;
}

void CDKGSessionManager::CleanupCache() const
{
    LOCK(contributionsCacheCs);
    auto curTime = GetTimeMillis();
    for (auto it = contributionsCache.begin(); it != contributionsCache.end(); ) {
        if (curTime - it->second.entryTime > MAX_CONTRIBUTION_CACHE_TIME) {
            it = contributionsCache.erase(it);
        } else {
            ++it;
        }
    }
}

bool IsQuorumDKGEnabled()
{
    return sporkManager.IsSporkActive(SPORK_17_QUORUM_DKG_ENABLED);
}

} // namespace llmq
