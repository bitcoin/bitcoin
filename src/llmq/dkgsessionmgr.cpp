// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/dkgsessionmgr.h>
#include <llmq/options.h>
#include <llmq/params.h>
#include <llmq/utils.h>

#include <bls/bls_ies.h>
#include <chainparams.h>
#include <dbwrapper.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <spork.h>
#include <unordered_lru_cache.h>
#include <util/irange.h>
#include <util/underlying.h>
#include <validation.h>

static bool IsQuorumDKGEnabled(const CSporkManager& sporkman)
{
    return sporkman.IsSporkActive(SPORK_17_QUORUM_DKG_ENABLED);
}

namespace llmq
{
static const std::string DB_VVEC = "qdkg_V";
static const std::string DB_SKCONTRIB = "qdkg_S";
static const std::string DB_ENC_CONTRIB = "qdkg_E";

CDKGSessionManager::CDKGSessionManager(CBLSWorker& _blsWorker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                                       CDKGDebugManager& _dkgDebugManager, CMasternodeMetaMan& mn_metaman,
                                       CQuorumBlockProcessor& _quorumBlockProcessor, CQuorumSnapshotManager& qsnapman,
                                       const CActiveMasternodeManager* const mn_activeman,
                                       const CSporkManager& sporkman, bool unitTests, bool fWipe) :
    db(std::make_unique<CDBWrapper>(unitTests ? "" : (gArgs.GetDataDirNet() / "llmq/dkgdb"), 1 << 20, unitTests, fWipe)),
    blsWorker(_blsWorker),
    m_chainstate(chainstate),
    m_dmnman(dmnman),
    dkgDebugManager(_dkgDebugManager),
    quorumBlockProcessor(_quorumBlockProcessor),
    m_qsnapman(qsnapman),
    spork_manager(sporkman)
{
    if (mn_activeman == nullptr && !IsWatchQuorumsEnabled()) {
        // Regular nodes do not care about any DKG internals, bail out
        return;
    }

    const Consensus::Params& consensus_params = Params().GetConsensus();
    for (const auto& params : consensus_params.llmqs) {
        auto session_count = (params.useRotation) ? params.signingActiveQuorumCount : 1;
        for (const auto i : irange::range(session_count)) {
            dkgSessionHandlers.emplace(std::piecewise_construct, std::forward_as_tuple(params.type, i),
                                       std::forward_as_tuple(blsWorker, m_chainstate, dmnman, dkgDebugManager, *this,
                                                             mn_metaman, quorumBlockProcessor, m_qsnapman, mn_activeman,
                                                             spork_manager, params, i));
        }
    }
}

CDKGSessionManager::~CDKGSessionManager() = default;

void CDKGSessionManager::StartThreads(CConnman& connman, PeerManager& peerman)
{
    for (auto& it : dkgSessionHandlers) {
        it.second.StartThread(connman, peerman);
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
    if (!DeploymentDIP0003Enforced(pindexNew->nHeight, Params().GetConsensus()))
        return;
    if (!IsQuorumDKGEnabled(spork_manager))
        return;

    for (auto& qt : dkgSessionHandlers) {
        qt.second.UpdatedBlockTip(pindexNew);
    }
}

PeerMsgRet CDKGSessionManager::ProcessMessage(CNode& pfrom, PeerManager& peerman, bool is_masternode,
                                              const std::string& msg_type, CDataStream& vRecv)
{
    static Mutex cs_indexedQuorumsCache;
    static std::map<Consensus::LLMQType, unordered_lru_cache<uint256, int, StaticSaltedHasher>> indexedQuorumsCache GUARDED_BY(cs_indexedQuorumsCache);

    if (!IsQuorumDKGEnabled(spork_manager))
        return {};

    if (msg_type != NetMsgType::QCONTRIB
        && msg_type != NetMsgType::QCOMPLAINT
        && msg_type != NetMsgType::QJUSTIFICATION
        && msg_type != NetMsgType::QPCOMMITMENT
        && msg_type != NetMsgType::QWATCH) {
        return {};
    }

    if (msg_type == NetMsgType::QWATCH) {
        if (!is_masternode) {
            // non-masternodes should never receive this
            return tl::unexpected{10};
        }
        pfrom.qwatch = true;
        return {};
    }

    if ((!is_masternode && !IsWatchQuorumsEnabled())) {
        // regular non-watching nodes should never receive any of these
        return tl::unexpected{10};
    }

    if (vRecv.empty()) {
        return tl::unexpected{100};
    }

    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    vRecv >> llmqType;
    vRecv >> quorumHash;
    vRecv.Rewind(sizeof(uint256));
    vRecv.Rewind(sizeof(uint8_t));

    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrintf("CDKGSessionManager -- invalid llmqType [%d]\n", ToUnderlying(llmqType));
        return tl::unexpected{100};
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
        CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(quorumHash));
        if (pQuorumBaseBlockIndex == nullptr) {
            LogPrintf("CDKGSessionManager -- unknown quorumHash %s\n", quorumHash.ToString());
            // NOTE: do not insta-ban for this, we might be lagging behind
            return tl::unexpected{10};
        }

        if (!IsQuorumTypeEnabled(llmqType, pQuorumBaseBlockIndex->pprev)) {
            LogPrintf("CDKGSessionManager -- llmqType [%d] quorums aren't active\n", ToUnderlying(llmqType));
            return tl::unexpected{100};
        }

        quorumIndex = pQuorumBaseBlockIndex->nHeight % llmq_params.dkgInterval;
        int quorumIndexMax = IsQuorumRotationEnabled(llmq_params, pQuorumBaseBlockIndex) ?
                llmq_params.signingActiveQuorumCount - 1 : 0;

        if (quorumIndex > quorumIndexMax) {
            LogPrintf("CDKGSessionManager -- invalid quorumHash %s\n", quorumHash.ToString());
            return tl::unexpected{100};
        }

        if (!dkgSessionHandlers.count(std::make_pair(llmqType, quorumIndex))) {
            LogPrintf("CDKGSessionManager -- no session handlers for quorumIndex [%d]\n", quorumIndex);
            return tl::unexpected{100};
        }
    }

    assert(quorumIndex != -1);
    WITH_LOCK(cs_indexedQuorumsCache, indexedQuorumsCache[llmqType].insert(quorumHash, quorumIndex));
    dkgSessionHandlers.at(std::make_pair(llmqType, quorumIndex)).ProcessMessage(pfrom, peerman, msg_type, vRecv);
    return {};
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
        LOCK(dkgType.cs_phase_qhash);
        if (dkgType.phase < QuorumPhase::Initialized || dkgType.phase > QuorumPhase::Contribute) {
            continue;
        }
        if (dkgType.GetContribution(hash, ret)) {
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
        LOCK(dkgType.cs_phase_qhash);
        if (dkgType.phase < QuorumPhase::Contribute || dkgType.phase > QuorumPhase::Complain) {
            continue;
        }
        if (dkgType.GetComplaint(hash, ret)) {
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
        LOCK(dkgType.cs_phase_qhash);
        if (dkgType.phase < QuorumPhase::Complain || dkgType.phase > QuorumPhase::Justify) {
            continue;
        }
        if (dkgType.GetJustification(hash, ret)) {
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
        LOCK(dkgType.cs_phase_qhash);
        if (dkgType.phase < QuorumPhase::Justify || dkgType.phase > QuorumPhase::Commit) {
            continue;
        }
        if (dkgType.GetPrematureCommitment(hash, ret)) {
            return true;
        }
    }
    return false;
}

void CDKGSessionManager::WriteVerifiedVvecContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const BLSVerificationVectorPtr& vvec)
{
    CDataStream s(SER_DISK, CLIENT_VERSION);
    WriteCompactSize(s, vvec->size());
    for (auto& pubkey : *vvec) {
        s << CBLSPublicKeyVersionWrapper(pubkey, false);
    }
    db->Write(std::make_tuple(DB_VVEC, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), s);
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
    auto members = utils::GetAllQuorumMembers(llmqType, m_dmnman, m_qsnapman, pQuorumBaseBlockIndex);

    memberIndexesRet.clear();
    vvecsRet.clear();
    skContributionsRet.clear();
    memberIndexesRet.reserve(members.size());
    vvecsRet.reserve(members.size());
    skContributionsRet.reserve(members.size());

    // NOTE: the `cs_main` should not be locked under scope of `contributionsCacheCs`
    LOCK(contributionsCacheCs);
    for (const auto i : irange::range(members.size())) {
        if (validMembers[i]) {
            const uint256& proTxHash = members[i]->proTxHash;
            ContributionsCacheKey cacheKey = {llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash};
            auto it = contributionsCache.find(cacheKey);
            if (it == contributionsCache.end()) {
                CDataStream s(SER_DISK, CLIENT_VERSION);
                if (!db->ReadDataStream(std::make_tuple(DB_VVEC, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), s)) {
                    LogPrint(BCLog::LLMQ, "%s -- this node does not have vvec for llmq=%d block=%s protx=%s\n",
                             __func__, ToUnderlying(llmqType), pQuorumBaseBlockIndex->GetBlockHash().ToString(),
                             proTxHash.ToString());
                    return false;
                }
                size_t vvec_size = ReadCompactSize(s);
                CBLSPublicKey pubkey;
                std::vector<CBLSPublicKey> qv;
                for ([[maybe_unused]] size_t _ : irange::range(vvec_size)) {
                    s >> CBLSPublicKeyVersionWrapper(pubkey, false);
                    qv.emplace_back(pubkey);
                }
                auto vvecPtr = std::make_shared<std::vector<CBLSPublicKey>>(std::move(qv));

                CBLSSecretKey skContribution;
                db->Read(std::make_tuple(DB_SKCONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), skContribution);

                it = contributionsCache.emplace(cacheKey, ContributionsCacheEntry{TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()), vvecPtr, skContribution}).first;
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
    auto members = utils::GetAllQuorumMembers(llmqType, m_dmnman, m_qsnapman, pQuorumBaseBlockIndex);

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
        LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- not a member, nProTxHash=%s\n", __func__, nProTxHash.ToString());
        return false;
    }

    for (const auto i : irange::range(members.size())) {
        if (validMembers[i]) {
            CBLSIESMultiRecipientObjects<CBLSSecretKey> encryptedContributions;
            if (!db->Read(std::make_tuple(DB_ENC_CONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), members[i]->proTxHash), encryptedContributions)) {
                LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- can't read from db, nProTxHash=%s\n", __func__, nProTxHash.ToString());
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

void CDKGSessionManager::CleanupOldContributions() const
{
    if (db->IsEmpty()) {
        return;
    }

    const auto prefixes = {DB_VVEC, DB_SKCONTRIB, DB_ENC_CONTRIB};

    for (const auto& params : Params().GetConsensus().llmqs) {
        LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- looking for old entries for llmq type %d\n", __func__, ToUnderlying(params.type));

        CDBBatch batch(*db);
        size_t cnt_old{0}, cnt_all{0};
        for (const auto& prefix : prefixes) {
            std::unique_ptr<CDBIterator> pcursor(db->NewIterator());
            auto start = std::make_tuple(prefix, params.type, uint256(), uint256());
            decltype(start) k;

            pcursor->Seek(start);
            LOCK(::cs_main);
            while (pcursor->Valid()) {
                if (!pcursor->GetKey(k) || std::get<0>(k) != prefix || std::get<1>(k) != params.type) {
                    break;
                }
                cnt_all++;
                const CBlockIndex* pindexQuorum = m_chainstate.m_blockman.LookupBlockIndex(std::get<2>(k));
                if (pindexQuorum == nullptr || m_chainstate.m_chain.Tip()->nHeight - pindexQuorum->nHeight > params.max_store_depth()) {
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

} // namespace llmq
