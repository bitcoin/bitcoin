// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/dkgsessionmgr.h>

#include <bls/bls_ies.h>
#include <evo/deterministicmns.h>
#include <llmq/dkgsessionhandler.h>
#include <llmq/options.h>
#include <llmq/utils.h>
#include <spork.h>
#include <unordered_lru_cache.h>
#include <util/helpers.h>
#include <util/std23.h>

#include <chainparams.h>
#include <dbwrapper.h>
#include <deploymentstatus.h>
#include <validation.h>

namespace llmq
{
static const std::string DB_VVEC = "qdkg_V";
static const std::string DB_SKCONTRIB = "qdkg_S";
static const std::string DB_ENC_CONTRIB = "qdkg_E";

CDKGSessionManager::CDKGSessionManager(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                                       const ChainstateManager& chainman, const CSporkManager& sporkman,
                                       const util::DbWrapperParams& db_params) :
    m_dmnman{dmnman},
    m_qsnapman{qsnapman},
    m_chainman{chainman},
    m_sporkman{sporkman},
    db{util::MakeDbWrapper({db_params.path / "llmq" / "dkgdb", db_params.memory, db_params.wipe, /*cache_size=*/1 << 20})}
{
}

CDKGSessionManager::~CDKGSessionManager() = default;

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
    db->Write(std::make_tuple(DB_VVEC, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), Span{s});

    // Reading the vvec back at commit time costs a point decompression and subgroup check per coefficient
    LOCK(contributionsCacheCs);
    auto& entry = contributionsCache[{llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash}];
    entry.entryTime = SteadyClock::now();
    entry.vvec = vvec;
}

void CDKGSessionManager::WriteVerifiedSkContribution(Consensus::LLMQType llmqType, const CBlockIndex* pQuorumBaseBlockIndex, const uint256& proTxHash, const CBLSSecretKey& skContribution)
{
    db->Write(std::make_tuple(DB_SKCONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), skContribution);

    LOCK(contributionsCacheCs);
    if (auto it = contributionsCache.find({llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash}); it != contributionsCache.end()) {
        it->second.skContribution = skContribution;
    }
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
    for (const auto i : util::irange(members.size())) {
        if (validMembers[i]) {
            const uint256& proTxHash = members[i]->proTxHash;
            ContributionsCacheKey cacheKey = {llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash};
            auto it = contributionsCache.find(cacheKey);
            if (it == contributionsCache.end()) {
                CDataStream s(SER_DISK, CLIENT_VERSION);
                if (!db->ReadDataStream(std::make_tuple(DB_VVEC, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), s)) {
                    LogPrint(BCLog::LLMQ, "%s -- this node does not have vvec for llmq=%d block=%s protx=%s\n",
                             __func__, std23::to_underlying(llmqType), pQuorumBaseBlockIndex->GetBlockHash().ToString(),
                             proTxHash.ToString());
                    return false;
                }
                size_t vvec_size = ReadCompactSize(s);
                CBLSPublicKey pubkey;
                std::vector<CBLSPublicKey> qv;
                for ([[maybe_unused]] size_t _ : util::irange(vvec_size)) {
                    s >> CBLSPublicKeyVersionWrapper(pubkey, false);
                    qv.emplace_back(pubkey);
                }
                auto vvecPtr = std::make_shared<std::vector<CBLSPublicKey>>(std::move(qv));

                CBLSSecretKey skContribution;
                db->Read(std::make_tuple(DB_SKCONTRIB, llmqType, pQuorumBaseBlockIndex->GetBlockHash(), proTxHash), skContribution);

                it = contributionsCache.emplace(cacheKey, ContributionsCacheEntry{SteadyClock::now(), vvecPtr, skContribution}).first;
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
    for (const auto i : util::irange(members.size())) {
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

    for (const auto i : util::irange(members.size())) {
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
    const auto curTime = SteadyClock::now();
    // A vvec still referenced elsewhere (by its DKG session) costs nothing to keep and is expensive to reload
    std::erase_if(contributionsCache, [&](const auto& entry) {
        return entry.second.vvec.use_count() == 1 && curTime - entry.second.entryTime > MAX_CONTRIBUTION_CACHE_TIME;
    });
}

void CDKGSessionManager::CleanupOldContributions() const
{
    if (db->IsEmpty()) {
        return;
    }

    const auto prefixes = {DB_VVEC, DB_SKCONTRIB, DB_ENC_CONTRIB};

    for (const auto& params : Params().GetConsensus().llmqs) {
        LogPrint(BCLog::LLMQ, "CDKGSessionManager::%s -- looking for old entries for llmq type %d\n", __func__, std23::to_underlying(params.type));

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
