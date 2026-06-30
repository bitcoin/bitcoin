// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/dkgsessionmgr.h>
#include <llmq/dkgmessages.h>
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

namespace {
// Upper bound on the serialized size of a well-formed DKG message of the given
// type for the given quorum params. Used to reject oversized payloads at intake
// before any deserialization or retention, which closes the low-cost memory
// amplification window (a legitimate message is bounded by quorum params, far
// below the 3 MiB transport cap). Generous slack is added and the result is
// clamped to a hard ceiling so a future params change can never silently re-open
// the full transport window.
size_t MaxDKGMessageSize(std::string_view msg_type, const Consensus::LLMQParams& params)
{
    constexpr size_t COMPACT = 5;          // max CompactSize for any realistic count
    constexpr size_t PREFIX = 1 + 32 + 32; // llmqType + quorumHash + proTxHash
    constexpr size_t PUBKEY = BLS_CURVE_PUBKEY_SIZE; // 48
    constexpr size_t SIG = BLS_CURVE_SIG_SIZE;       // 96
    constexpr size_t SECKEY = BLS_CURVE_SECKEY_SIZE; // 32
    constexpr size_t BLOB = COMPACT + 128; // encrypted seckey blob, generous
    constexpr size_t SLACK = 1024;
    constexpr size_t HARD_CEILING = size_t{1} << 20; // 1 MiB

    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    const size_t threshold = params.threshold > 0 ? static_cast<size_t>(params.threshold) : 0;

    size_t cap = 0;
    if (msg_type == NetMsgType::QCONTRIB) {
        // llmqType/quorumHash/proTxHash + vvec + contributions(IES) + sig
        cap = PREFIX + (COMPACT + threshold * PUBKEY) + (PUBKEY + 32 + COMPACT + size * BLOB) + SIG;
    } else if (msg_type == NetMsgType::QJUSTIFICATION) {
        // ... + contributions(index u32 + seckey) + sig
        cap = PREFIX + (COMPACT + size * (4 + SECKEY)) + SIG;
    } else if (msg_type == NetMsgType::QCOMPLAINT) {
        // ... + 2 dynamic bitsets (badMembers, complainForMembers) + sig
        cap = PREFIX + 2 * (COMPACT + (size + 7) / 8) + SIG;
    } else if (msg_type == NetMsgType::QPCOMMITMENT) {
        // ... + validMembers bitset + quorumPublicKey + quorumVvecHash + quorumSig + sig
        cap = PREFIX + (COMPACT + (size + 7) / 8) + PUBKEY + 32 + 2 * SIG;
    } else {
        return HARD_CEILING;
    }
    cap += SLACK;
    return cap < HARD_CEILING ? cap : HARD_CEILING;
}

// Cheap, param-only structural validation of a pushed DKG message, run at intake
// before retention. Deserializes a COPY of the payload (leaving the caller's bytes
// intact for the pending queue and its inventory hash) and checks only invariants
// derived from quorum params: no member-list lookup and no signature verification,
// which remain on the DKG worker thread. Deserializing the copy does decompress the
// BLS points carried in the payload, but that work is bounded by the size cap applied
// just before this check. Rejects malformed or wrong-shaped payloads before retention.
bool CheckDKGMessageStructure(std::string_view msg_type, const CDataStream& vRecv, const Consensus::LLMQParams& params)
{
    const size_t size = params.size > 0 ? static_cast<size_t>(params.size) : 0;
    const size_t threshold = params.threshold > 0 ? static_cast<size_t>(params.threshold) : 0;
    try {
        CDataStream s(vRecv); // copy; deserialization does not advance the caller's stream
        if (msg_type == NetMsgType::QCONTRIB) {
            CDKGContribution qc;
            s >> qc;
            return qc.vvec != nullptr && qc.vvec->size() == threshold &&
                   qc.contributions != nullptr && qc.contributions->blobs.size() == size;
        } else if (msg_type == NetMsgType::QCOMPLAINT) {
            CDKGComplaint qc;
            s >> qc;
            return qc.badMembers.size() == size && qc.complainForMembers.size() == size;
        } else if (msg_type == NetMsgType::QJUSTIFICATION) {
            CDKGJustification qj;
            s >> qj;
            return qj.contributions.size() <= size;
        } else if (msg_type == NetMsgType::QPCOMMITMENT) {
            CDKGPrematureCommitment qc;
            s >> qc;
            return qc.validMembers.size() == size;
        }
        return false;
    } catch (const std::exception&) {
        return false;
    }
}
} // anonymous namespace

CDKGSessionManager::CDKGSessionManager(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                                       const ChainstateManager& chainman, const CSporkManager& sporkman,
                                       const util::DbWrapperParams& db_params, bool quorums_watch) :
    m_dmnman{dmnman},
    m_qsnapman{qsnapman},
    m_chainman{chainman},
    m_sporkman{sporkman},
    m_quorums_watch{quorums_watch},
    db{util::MakeDbWrapper({db_params.path / "llmq" / "dkgdb", db_params.memory, db_params.wipe, /*cache_size=*/1 << 20})}
{
}

CDKGSessionManager::~CDKGSessionManager() = default;

void CDKGSessionManager::StartThreads(CConnman& connman, PeerManager& peerman)
{
    for (auto& [_, dkgType] : dkgSessionHandlers) {
        Assert(dkgType)->StartThread(connman, peerman);
    }
}

void CDKGSessionManager::StopThreads()
{
    for (auto& [_, dkgType] : dkgSessionHandlers) {
        Assert(dkgType)->StopThread();
    }
}

void CDKGSessionManager::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
{
    CleanupCache();

    if (fInitialDownload)
        return;
    if (!DeploymentDIP0003Enforced(pindexNew->nHeight, Params().GetConsensus()))
        return;
    if (!IsQuorumDKGEnabled(m_sporkman))
        return;

    for (auto& [_, dkgType] : dkgSessionHandlers) {
        Assert(dkgType)->UpdatedBlockTip(pindexNew);
    }
}

MessageProcessingResult CDKGSessionManager::ProcessMessage(CNode& pfrom, bool is_masternode, std::string_view msg_type,
                                                           CDataStream& vRecv)
{
    static Mutex cs_indexedQuorumsCache;
    static std::map<Consensus::LLMQType, Uint256LruHashMap<int>> indexedQuorumsCache GUARDED_BY(cs_indexedQuorumsCache);

    if (!IsQuorumDKGEnabled(m_sporkman))
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
            return MisbehavingError{10};
        }
        pfrom.qwatch = true;
        return {};
    }

    if (!is_masternode && !m_quorums_watch) {
        // regular non-watching nodes should never receive any of these
        return MisbehavingError{10};
    }

    // Pushed DKG messages (QCONTRIB/QCOMPLAINT/QJUSTIFICATION/QPCOMMITMENT) retain
    // attacker-controlled payloads, so they must originate from an MNAuth-verified
    // masternode. qwatch is unauthenticated (any peer can set it via QWATCH) and is
    // only meaningful for pull/observation paths; it must not bypass this gate.
    if (pfrom.GetVerifiedProRegTxHash().IsNull()) {
        return MisbehavingError{10, "DKG message from non-verified peer"};
    }

    if (vRecv.empty()) {
        return MisbehavingError{100};
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
        return MisbehavingError{100};
    }
    const auto& llmq_params = llmq_params_opt.value();

    int quorumIndex{-1};

    // First check cache
    {
        LOCK(cs_indexedQuorumsCache);
        if (indexedQuorumsCache.empty()) {
            utils::InitQuorumsCache(indexedQuorumsCache, m_chainman.GetConsensus());
        }
        indexedQuorumsCache[llmqType].get(quorumHash, quorumIndex);
    }

    // No luck, try to compute
    if (quorumIndex == -1) {
        const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(::cs_main,
                                                             return m_chainman.m_blockman.LookupBlockIndex(quorumHash));
        if (pQuorumBaseBlockIndex == nullptr) {
            LogPrintf("CDKGSessionManager -- unknown quorumHash %s\n", quorumHash.ToString());
            // NOTE: do not insta-ban for this, we might be lagging behind
            return MisbehavingError{10};
        }

        if (!m_chainman.IsQuorumTypeEnabled(llmqType, pQuorumBaseBlockIndex->pprev)) {
            LogPrintf("CDKGSessionManager -- llmqType [%d] quorums aren't active\n", ToUnderlying(llmqType));
            return MisbehavingError{100};
        }

        quorumIndex = pQuorumBaseBlockIndex->nHeight % llmq_params.dkgInterval;
        int quorumIndexMax = IsQuorumRotationEnabled(llmq_params, pQuorumBaseBlockIndex) ?
                llmq_params.signingActiveQuorumCount - 1 : 0;

        if (quorumIndex > quorumIndexMax) {
            LogPrintf("CDKGSessionManager -- invalid quorumHash %s\n", quorumHash.ToString());
            return MisbehavingError{100};
        }

        if (!dkgSessionHandlers.count({llmqType, quorumIndex})) {
            LogPrintf("CDKGSessionManager -- no session handlers for quorumIndex [%d]\n", quorumIndex);
            return MisbehavingError{100};
        }
    }

    assert(quorumIndex != -1);

    // Reject oversized payloads before any deserialization or retention. A
    // well-formed DKG message is bounded by quorum params; anything larger is an
    // amplification attempt against the per-peer pending queue.
    if (vRecv.size() > MaxDKGMessageSize(msg_type, llmq_params)) {
        return MisbehavingError{100, "oversized DKG message"};
    }

    // Cheap structural pre-validation before retention. Validates a copy so the
    // original bytes (and their inventory hash) are preserved for the worker.
    if (!CheckDKGMessageStructure(msg_type, vRecv, llmq_params)) {
        return MisbehavingError{100, "malformed DKG message"};
    }

    WITH_LOCK(cs_indexedQuorumsCache, indexedQuorumsCache[llmqType].insert(quorumHash, quorumIndex));
    return Assert(dkgSessionHandlers.at({llmqType, quorumIndex}))->ProcessMessage(pfrom.GetId(), msg_type, vRecv);
}

bool CDKGSessionManager::AlreadyHave(const CInv& inv) const
{
    if (!IsQuorumDKGEnabled(m_sporkman))
        return false;

    for (const auto& [_, dkgType] : dkgSessionHandlers) {
        if (Assert(dkgType)->pendingContributions.HasSeen(inv.hash)
            || dkgType->pendingComplaints.HasSeen(inv.hash)
            || dkgType->pendingJustifications.HasSeen(inv.hash)
            || dkgType->pendingPrematureCommitments.HasSeen(inv.hash)) {
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetContribution(const uint256& hash, CDKGContribution& ret) const
{
    if (!IsQuorumDKGEnabled(m_sporkman))
        return false;

    for (const auto& [_, dkgType] : dkgSessionHandlers) {
        const auto dkgPhase = Assert(dkgType)->GetPhase();
        if (dkgPhase < QuorumPhase::Initialized || dkgPhase > QuorumPhase::Contribute) {
            continue;
        }
        if (dkgType->GetContribution(hash, ret)) {
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetComplaint(const uint256& hash, CDKGComplaint& ret) const
{
    if (!IsQuorumDKGEnabled(m_sporkman))
        return false;

    for (const auto& [_, dkgType] : dkgSessionHandlers) {
        const auto dkgPhase = Assert(dkgType)->GetPhase();
        if (dkgPhase < QuorumPhase::Contribute || dkgPhase > QuorumPhase::Complain) {
            continue;
        }
        if (dkgType->GetComplaint(hash, ret)) {
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetJustification(const uint256& hash, CDKGJustification& ret) const
{
    if (!IsQuorumDKGEnabled(m_sporkman))
        return false;

    for (const auto& [_, dkgType] : dkgSessionHandlers) {
        const auto dkgPhase = Assert(dkgType)->GetPhase();
        if (dkgPhase < QuorumPhase::Complain || dkgPhase > QuorumPhase::Justify) {
            continue;
        }
        if (dkgType->GetJustification(hash, ret)) {
            return true;
        }
    }
    return false;
}

bool CDKGSessionManager::GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const
{
    if (!IsQuorumDKGEnabled(m_sporkman))
        return false;

    for (const auto& [_, dkgType] : dkgSessionHandlers) {
        const auto dkgPhase = Assert(dkgType)->GetPhase();
        if (dkgPhase < QuorumPhase::Justify || dkgPhase > QuorumPhase::Commit) {
            continue;
        }
        if (dkgType->GetPrematureCommitment(hash, ret)) {
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
    auto members = utils::GetAllQuorumMembers(llmqType, {m_dmnman, m_qsnapman, m_chainman, pQuorumBaseBlockIndex});

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
    auto members = utils::GetAllQuorumMembers(llmqType, {m_dmnman, m_qsnapman, m_chainman, pQuorumBaseBlockIndex});

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
                const CBlockIndex* pindexQuorum = m_chainman.m_blockman.LookupBlockIndex(std::get<2>(k));
                if (pindexQuorum == nullptr ||
                    m_chainman.ActiveHeight() - pindexQuorum->nHeight > params.max_store_depth()) {
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
