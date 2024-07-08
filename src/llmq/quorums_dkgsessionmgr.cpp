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
#include <common/args.h>
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
    dkgSessionHandler = std::make_unique<CDKGSessionHandler>(
        blsWorker, 
        *this, 
        _peerman, 
        _chainman
    );
}

void CDKGSessionManager::StartThreads()
{
    if (!fMasternodeMode && !CLLMQUtils::IsWatchQuorumsEnabled()) {
        // Regular nodes do not care about any DKG internals, bail out
        return;
    }
    dkgSessionHandler->StartThread();
}

void CDKGSessionManager::StopThreads()
{
    dkgSessionHandler->StopThread();
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

    dkgSessionHandler->UpdatedBlockTip(pindexNew);
    
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

    dkgSessionHandler->ProcessMessage(pfrom, strCommand, vRecv);
}

bool CDKGSessionManager::AlreadyHave(const uint256& hash) const
{
    if (!IsQuorumDKGEnabled())
        return false;
    
    
    LOCK(dkgSessionHandler->cs);   
    if (dkgSessionHandler->pendingContributions.HasSeen(hash)
        || dkgSessionHandler->pendingComplaints.HasSeen(hash)
        || dkgSessionHandler->pendingJustifications.HasSeen(hash)
        || dkgSessionHandler->pendingPrematureCommitments.HasSeen(hash)) {
        return true;
    }
    
    return false;
}

bool CDKGSessionManager::GetContribution(const uint256& hash, CDKGContribution& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;

    
    LOCK(dkgSessionHandler->cs);
    if (dkgSessionHandler->phase < QuorumPhase_Initialized || dkgSessionHandler->phase > QuorumPhase_Contribute) {
        return false;
    }
    LOCK(dkgSessionHandler->curSession->invCs);
    auto it = dkgSessionHandler->curSession->contributions.find(hash);
    if (it != dkgSessionHandler->curSession->contributions.end()) {
        ret = it->second;
        return true;
    }
    return false;
}

bool CDKGSessionManager::GetComplaint(const uint256& hash, CDKGComplaint& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;

    
    LOCK(dkgSessionHandler->cs);
    if (dkgSessionHandler->phase < QuorumPhase_Contribute || dkgSessionHandler->phase > QuorumPhase_Complain) {
        return false;
    }
    LOCK(dkgSessionHandler->curSession->invCs);
    auto it = dkgSessionHandler->curSession->complaints.find(hash);
    if (it != dkgSessionHandler->curSession->complaints.end()) {
        ret = it->second;
        return true;
    }
    return false;
}

bool CDKGSessionManager::GetJustification(const uint256& hash, CDKGJustification& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;

    
    LOCK(dkgSessionHandler->cs);
    if (dkgSessionHandler->phase < QuorumPhase_Complain || dkgSessionHandler->phase > QuorumPhase_Justify) {
        return false;
    }
    LOCK(dkgSessionHandler->curSession->invCs);
    auto it = dkgSessionHandler->curSession->justifications.find(hash);
    if (it != dkgSessionHandler->curSession->justifications.end()) {
        ret = it->second;
        return true;
    }
    return false;
}

bool CDKGSessionManager::GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const
{
    if (!IsQuorumDKGEnabled())
        return false;


    
    LOCK(dkgSessionHandler->cs);
    if (dkgSessionHandler->phase < QuorumPhase_Justify || dkgSessionHandler->phase > QuorumPhase_Commit) {
        return false;
    }
    LOCK(dkgSessionHandler->curSession->invCs);
    auto it = dkgSessionHandler->curSession->prematureCommitments.find(hash);
    if (it != dkgSessionHandler->curSession->prematureCommitments.end() && dkgSessionHandler->curSession->validCommitments.count(hash)) {
        ret = it->second;
        return true;
    }

    return false;
}

void CDKGSessionManager::WriteVerifiedVvecContribution(const uint256& hashQuorum, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec)
{
    db->Write(std::make_tuple(DB_VVEC, hashQuorum, proTxHash), *vvec);
}

void CDKGSessionManager::WriteVerifiedSkContribution(const uint256& hashQuorum, const uint256& proTxHash, const CBLSSecretKey& skContribution)
{
    db->Write(std::make_tuple(DB_SKCONTRIB, hashQuorum, proTxHash), skContribution);
}
void CDKGSessionManager::WriteEncryptedContributions(const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSIESMultiRecipientObjects<CBLSSecretKey>& contributions)
{
    db->Write(std::make_tuple(DB_ENC_CONTRIB, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), contributions);
}
bool CDKGSessionManager::GetVerifiedContributions(const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, std::vector<uint16_t>& memberIndexesRet, std::vector<BLSVerificationVectorPtr>& vvecsRet, std::vector<CBLSSecretKey>& skContributionsRet) const
{
    LOCK(contributionsCacheCs);
    auto members = CLLMQUtils::GetAllQuorumMembers(pQuorumBaseBlockIndex);

    memberIndexesRet.clear();
    vvecsRet.clear();
    skContributionsRet.clear();
    memberIndexesRet.reserve(members.size());
    vvecsRet.reserve(members.size());
    skContributionsRet.reserve(members.size());
    for (size_t i = 0; i < members.size(); i++) {
        if (validMembers[i]) {
            const uint256& proTxHash = members[i]->proTxHash;
            ContributionsCacheKey cacheKey = {pQuorumBaseBlockIndex->GetBlockHash(), proTxHash};
            auto it = contributionsCache.find(cacheKey);
            if (it == contributionsCache.end()) {
                auto vvecPtr = std::make_shared<std::vector<CBLSPublicKey>>();
                CBLSSecretKey skContribution;
                if (!db->Read(std::make_tuple(DB_VVEC, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), *vvecPtr)) {
                    return false;
                }
                db->Read(std::make_tuple(DB_SKCONTRIB, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), skContribution);

                it = contributionsCache.emplace(cacheKey, ContributionsCacheEntry{TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()), vvecPtr, skContribution}).first;
            }

            memberIndexesRet.emplace_back(i);
            vvecsRet.emplace_back(it->second.vvec);
            skContributionsRet.emplace_back(it->second.skContribution);
        }
    }
    return true;
}

bool CDKGSessionManager::GetEncryptedContributions(const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, const uint256& nProTxHash, std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vecRet) const
{
    auto members = CLLMQUtils::GetAllQuorumMembers(pQuorumBaseBlockIndex);

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
            if (!db->Read(std::make_tuple(DB_ENC_CONTRIB, pQuorumBaseBlockIndex->GetBlockHash(), members[i]->proTxHash), encryptedContributions)) {
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
    auto curTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    for (auto it = contributionsCache.begin(); it != contributionsCache.end(); ) {
        if (curTime - it->second.entryTime > MAX_CONTRIBUTION_CACHE_TIME) {
            it = contributionsCache.erase(it);
        } else {
            ++it;
        }
    }
}

void CDKGSessionManager::CleanupOldContributions(ChainstateManager& chainstate) const
{
    if (db->IsEmpty()) {
        return;
    }

    const auto prefixes = {DB_VVEC, DB_SKCONTRIB, DB_ENC_CONTRIB};

    LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- looking for old entries\n", __func__);
    auto &params = Params().GetConsensus().llmqTypeChainLocks;
    CDBBatch batch(*db);
    size_t cnt_old{0}, cnt_all{0};
    for (const auto& prefix : prefixes) {
        std::unique_ptr<CDBIterator> pcursor(db->NewIterator());
        auto start = std::make_tuple(prefix, uint256(), uint256());
        decltype(start) k;

        pcursor->Seek(start);
        LOCK(cs_main);
        while (pcursor->Valid()) {
            if (!pcursor->GetKey(k) || std::get<0>(k) != prefix) {
                break;
            }
            cnt_all++;
            const CBlockIndex* pindexQuorum = chainstate.m_blockman.LookupBlockIndex(std::get<2>(k));
            if (pindexQuorum == nullptr || chainstate.ActiveHeight() - pindexQuorum->nHeight > params.max_store_depth()) {
                // not found or too old
                batch.Erase(k);
                cnt_old++;
            }
            pcursor->Next();
        }
        pcursor.reset();
    }
    LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- found %lld entries\n", __func__, cnt_all);
    if (cnt_old > 0) {
        db->WriteBatch(batch);
        LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- removed %lld old entries\n", __func__, cnt_old);
    }
}

bool IsQuorumDKGEnabled()
{
    return sporkManager->IsSporkActive(SPORK_17_QUORUM_DKG_ENABLED);
}

} // namespace llmq
