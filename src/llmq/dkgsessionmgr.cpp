// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/quorums.h>
#include <llmq/utils.h>

#include <evo/deterministicmns.h>

#include <chainparams.h>
#include <dbwrapper.h>
#include <net_processing.h>
#include <spork.h>
#include <util/irange.h>
#include <util/underlying.h>
#include <validation.h>

namespace llmq
{
static const std::string DB_VVEC = "qdkg_V";
static const std::string DB_SKCONTRIB = "qdkg_S";
static const std::string DB_ENC_CONTRIB = "qdkg_E";

CDKGSessionManager::CDKGSessionManager(CBLSWorker& _blsWorker, CChainState& chainstate, CConnman& _connman, CDKGDebugManager& _dkgDebugManager,
                                       CQuorumBlockProcessor& _quorumBlockProcessor, CSporkManager& sporkManager,
                                       const std::unique_ptr<PeerManager>& peerman, bool unitTests, bool fWipe) :
    db(std::make_unique<CDBWrapper>(unitTests ? "" : (GetDataDir() / "llmq/dkgdb"), 1 << 20, unitTests, fWipe)),
    blsWorker(_blsWorker),
    m_chainstate(chainstate),
    connman(_connman),
    dkgDebugManager(_dkgDebugManager),
    quorumBlockProcessor(_quorumBlockProcessor),
    spork_manager(sporkManager),
    m_peerman(peerman)
{
    if (!fMasternodeMode && !utils::IsWatchQuorumsEnabled()) {
        // Regular nodes do not care about any DKG internals, bail out
        return;
    }

    MigrateDKG();

    const Consensus::Params& consensus_params = Params().GetConsensus();
    for (const auto& params : consensus_params.llmqs) {
        auto session_count = (params.useRotation) ? params.signingActiveQuorumCount : 1;
        for (const auto i : irange::range(session_count)) {
            dkgSessionHandlers.emplace(std::piecewise_construct,
                                       std::forward_as_tuple(params.type, i),
                                       std::forward_as_tuple(blsWorker, m_chainstate, connman, dkgDebugManager, *this, quorumBlockProcessor, params, peerman, i));
        }
    }
}

void CDKGSessionManager::MigrateDKG()
{
    if (!db->IsEmpty()) return;

    LogPrint(BCLog::LLMQ, "CDKGSessionManager::%d -- start\n", __func__);

    CDBBatch batch(*db);
    auto oldDb = std::make_unique<CDBWrapper>(GetDataDir() / "llmq", 8 << 20);
    std::unique_ptr<CDBIterator> pcursor(oldDb->NewIterator());

    auto start_vvec = std::make_tuple(DB_VVEC, (Consensus::LLMQType)0, uint256(), uint256());
    pcursor->Seek(start_vvec);

    while (pcursor->Valid()) {
        decltype(start_vvec) k;
        std::vector<CBLSPublicKey> v;

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

    auto start_contrib = std::make_tuple(DB_SKCONTRIB, (Consensus::LLMQType)0, uint256(), uint256());
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

    auto start_enc_contrib = std::make_tuple(DB_ENC_CONTRIB, (Consensus::LLMQType)0, uint256(), uint256());
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
    if (!deterministicMNManager->IsDIP3Enforced(pindexNew->nHeight))
        return;
    if (!IsQuorumDKGEnabled(spork_manager))
        return;

    for (auto& qt : dkgSessionHandlers) {
        qt.second.UpdatedBlockTip(pindexNew);
    }
}

void CDKGSessionManager::ProcessMessage(CNode& pfrom, const CQuorumManager& quorum_manager, const std::string& msg_type, CDataStream& vRecv)
{
    static Mutex cs_indexedQuorumsCache;
    static std::map<Consensus::LLMQType, unordered_lru_cache<uint256, int, StaticSaltedHasher>> indexedQuorumsCache GUARDED_BY(cs_indexedQuorumsCache);

    if (!IsQuorumDKGEnabled(spork_manager))
        return;

    if (msg_type != NetMsgType::QCONTRIB
        && msg_type != NetMsgType::QCOMPLAINT
        && msg_type != NetMsgType::QJUSTIFICATION
        && msg_type != NetMsgType::QPCOMMITMENT
        && msg_type != NetMsgType::QWATCH) {
        return;
    }

    if (msg_type == NetMsgType::QWATCH) {
        pfrom.qwatch = true;
        return;
    }

    if (vRecv.empty()) {
        m_peerman->Misbehaving(pfrom.GetId(), 100);
        return;
    }

    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    vRecv >> llmqType;
    vRecv >> quorumHash;
    vRecv.Rewind(sizeof(uint256));
    vRecv.Rewind(sizeof(uint8_t));

    const auto& llmq_params_opt = GetLLMQParams(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrintf("CDKGSessionManager -- invalid llmqType [%d]\n", ToUnderlying(llmqType));
        m_peerman->Misbehaving(pfrom.GetId(), 100);
        return;
    }
    const auto& llmq_params = llmq_params_opt.value();

    int quorumIndex{-1};

    // First check cache
    {
        LOCK(cs_indexedQuorumsCache);
        if (indexedQuorumsCache.empty()) {
            utils::InitQuorumsCache(indexedQuorumsCache);
        }
        indexedQuorumsCache[llmqType].get(quorumHash, quorumIndex);
    }

    // No luck, try to compute
    if (quorumIndex == -1) {
        CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(cs_main, return m_chainstate.m_blockman.LookupBlockIndex(quorumHash));
        if (pQuorumBaseBlockIndex == nullptr) {
            LogPrintf("CDKGSessionManager -- unknown quorumHash %s\n", quorumHash.ToString());
            // NOTE: do not insta-ban for this, we might be lagging behind
            m_peerman->Misbehaving(pfrom.GetId(), 10);
            return;
        }

        if (!utils::IsQuorumTypeEnabled(llmqType, quorum_manager, pQuorumBaseBlockIndex->pprev)) {
            LogPrintf("CDKGSessionManager -- llmqType [%d] quorums aren't active\n", ToUnderlying(llmqType));
            m_peerman->Misbehaving(pfrom.GetId(), 100);
            return;
        }

        quorumIndex = pQuorumBaseBlockIndex->nHeight % llmq_params.dkgInterval;
        int quorumIndexMax = utils::IsQuorumRotationEnabled(llmq_params, pQuorumBaseBlockIndex) ?
                llmq_params.signingActiveQuorumCount - 1 : 0;

        if (quorumIndex > quorumIndexMax) {
            LogPrintf("CDKGSessionManager -- invalid quorumHash %s\n", quorumHash.ToString());
            m_peerman->Misbehaving(pfrom.GetId(), 100);
            return;
        }

        if (!dkgSessionHandlers.count(std::make_pair(llmqType, quorumIndex))) {
            LogPrintf("CDKGSessionManager -- no session handlers for quorumIndex [%d]\n", quorumIndex);
            m_peerman->Misbehaving(pfrom.GetId(), 100);
            return;
        }
    }

    assert(quorumIndex != -1);
    WITH_LOCK(cs_indexedQuorumsCache, indexedQuorumsCache[llmqType].insert(quorumHash, quorumIndex));
    dkgSessionHandlers.at(std::make_pair(llmqType, quorumIndex)).ProcessMessage(pfrom, msg_type, vRecv);
}

bool CDKGSessionManager::AlreadyHave(const CInv& inv) const
{
    if (!IsQuorumDKGEnabled(spork_manager))
        return false;

    for (const auto& p : dkgSessionHandlers) {
        const auto& dkgType = p.second;
        if (dkgType.pendingContributions.HasSeen(inv.hash)
            || dkgType.pendingComplaints.HasSeen(inv.hash)
            || dkgType.pendingJustifications.HasSeen(inv.hash)
            || dkgType.pendingPrematureCommitments.HasSeen(inv.hash)) {
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetContribution(const uint256& hash, CDKGContribution& ret) const
{
    if (!IsQuorumDKGEnabled(spork_manager))
        return false;

    for (const auto& p : dkgSessionHandlers) {
        const auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase::Initialized || dkgType.phase > QuorumPhase::Contribute) {
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
    if (!IsQuorumDKGEnabled(spork_manager))
        return false;

    for (const auto& p : dkgSessionHandlers) {
        const auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase::Contribute || dkgType.phase > QuorumPhase::Complain) {
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
    if (!IsQuorumDKGEnabled(spork_manager))
        return false;

    for (const auto& p : dkgSessionHandlers) {
        const auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase::Complain || dkgType.phase > QuorumPhase::Justify) {
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
    if (!IsQuorumDKGEnabled(spork_manager))
        return false;

    for (const auto& p : dkgSessionHandlers) {
        const auto& dkgType = p.second;
        LOCK(dkgType.cs);
        if (dkgType.phase < QuorumPhase::Justify || dkgType.phase > QuorumPhase::Commit) {
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

void CDKGSessionManager::WriteVerifiedVvecContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec)
{
    db->Write(std::make_tuple(DB_VVEC, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), *vvec);
}

void CDKGSessionManager::WriteVerifiedSkContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSSecretKey& skContribution)
{
    db->Write(std::make_tuple(DB_SKCONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), skContribution);
}

void CDKGSessionManager::WriteEncryptedContributions(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSIESMultiRecipientObjects<CBLSSecretKey>& contributions)
{
    db->Write(std::make_tuple(DB_ENC_CONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), contributions);
}

bool CDKGSessionManager::GetVerifiedContributions(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, std::vector<uint16_t>& memberIndexesRet, std::vector<BLSVerificationVectorPtr>& vvecsRet, std::vector<CBLSSecretKey>& skContributionsRet) const
{
    LOCK(contributionsCacheCs);
    auto members = utils::GetAllQuorumMembers(llmqType, pQuorumBaseBlockIndex);

    memberIndexesRet.clear();
    vvecsRet.clear();
    skContributionsRet.clear();
    memberIndexesRet.reserve(members.size());
    vvecsRet.reserve(members.size());
    skContributionsRet.reserve(members.size());
    for (const auto i : irange::range(members.size())) {
        if (validMembers[i]) {
            const uint256& proTxHash = members[i]->proTxHash;
            ContributionsCacheKey cacheKey = {llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash};
            auto it = contributionsCache.find(cacheKey);
            if (it == contributionsCache.end()) {
                auto vvecPtr = std::make_shared<std::vector<CBLSPublicKey>>();
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

bool CDKGSessionManager::GetEncryptedContributions(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const std::vector<bool>& validMembers, const uint256& nProTxHash, std::vector<CBLSIESEncryptedObject<CBLSSecretKey>>& vecRet) const
{
    auto members = utils::GetAllQuorumMembers(llmqType, pQuorumBaseBlockIndex);

    vecRet.clear();
    vecRet.reserve(members.size());

    size_t nRequestedMemberIdx{std::numeric_limits<size_t>::max()};
    for (const auto i : irange::range(members.size())) {
        // cppcheck-suppress useStlAlgorithm
        if (members[i]->proTxHash == nProTxHash) {
            nRequestedMemberIdx = i;
            break;
        }
    }
    if (nRequestedMemberIdx == std::numeric_limits<size_t>::max()) {
        return false;
    }

    for (const auto i : irange::range(members.size())) {
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

void CDKGSessionManager::CleanupOldContributions() const
{
    if (db->IsEmpty()) {
        return;
    }

    const auto prefixes = {DB_VVEC, DB_SKCONTRIB, DB_ENC_CONTRIB};

    for (const auto& params : Params().GetConsensus().llmqs) {
        // For how many blocks recent DKG info should be kept
        const int MAX_STORE_DEPTH = 2 * params.signingActiveQuorumCount * params.dkgInterval;

        LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- looking for old entries for llmq type %d\n", __func__, ToUnderlying(params.type));

        CDBBatch batch(*db);
        size_t cnt_old{0}, cnt_all{0};
        for (const auto& prefix : prefixes) {
            std::unique_ptr<CDBIterator> pcursor(db->NewIterator());
            auto start = std::make_tuple(prefix, params.type, uint256(), uint256());
            decltype(start) k;

            pcursor->Seek(start);
            LOCK(cs_main);
            while (pcursor->Valid()) {
                if (!pcursor->GetKey(k) || std::get<0>(k) != prefix || std::get<1>(k) != params.type) {
                    break;
                }
                cnt_all++;
                const CBlockIndex* pindexQuorum = m_chainstate.m_blockman.LookupBlockIndex(std::get<2>(k));
                if (pindexQuorum == nullptr || m_chainstate.m_chain.Tip()->nHeight - pindexQuorum->nHeight > MAX_STORE_DEPTH) {
                    // not found or too old
                    batch.Erase(k);
                    cnt_old++;
                }
                pcursor->Next();
            }
            pcursor.reset();
        }
        LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- found %lld entries for llmq type %d\n", __func__, cnt_all, uint8_t(params.type));
        if (cnt_old > 0) {
            db->WriteBatch(batch);
            LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- removed %lld old entries for llmq type %d\n", __func__, cnt_old, uint8_t(params.type));
        }
    }
}

bool IsQuorumDKGEnabled(const CSporkManager& sporkManager)
{
    return sporkManager.IsSporkActive(SPORK_17_QUORUM_DKG_ENABLED);
}

} // namespace llmq
