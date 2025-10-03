// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/meta.h>

#include <flat-database.h>
#include <univalue.h>
#include <util/time.h>

const std::string MasternodeMetaStore::SERIALIZATION_VERSION_STRING = "CMasternodeMetaMan-Version-4";

CMasternodeMetaMan::CMasternodeMetaMan() :
    m_db{std::make_unique<db_type>("mncache.dat", "magicMasternodeCache")}
{
}

bool CMasternodeMetaMan::LoadCache(bool load_cache)
{
    assert(m_db != nullptr);
    is_valid = load_cache ? m_db->Load(*this) : m_db->Store(*this);
    return is_valid;
}

CMasternodeMetaMan::~CMasternodeMetaMan()
{
    if (!is_valid) return;
    m_db->Store(*this);
}

UniValue CMasternodeMetaInfo::ToJson() const
{
    UniValue ret(UniValue::VOBJ);

    int64_t now = GetTime<std::chrono::seconds>().count();

    ret.pushKV("lastDSQ", nLastDsq.load());
    ret.pushKV("mixingTxCount", nMixingTxCount.load());
    ret.pushKV("outboundAttemptCount", outboundAttemptCount.load());
    ret.pushKV("lastOutboundAttempt", lastOutboundAttempt.load());
    ret.pushKV("lastOutboundAttemptElapsed", now - lastOutboundAttempt.load());
    ret.pushKV("lastOutboundSuccess", lastOutboundSuccess.load());
    ret.pushKV("lastOutboundSuccessElapsed", now - lastOutboundSuccess.load());
    {
        LOCK(cs);
        ret.pushKV("is_platform_banned", m_platform_ban);
        ret.pushKV("platform_ban_height_updated", m_platform_ban_updated);
    }

    return ret;
}

void CMasternodeMetaInfo::AddGovernanceVote(const uint256& nGovernanceObjectHash)
{
    LOCK(cs);
    // Insert a zero value, or not. Then increment the value regardless. This
    // ensures the value is in the map.
    const auto& pair = mapGovernanceObjectsVotedOn.emplace(nGovernanceObjectHash, 0);
    pair.first->second++;
}

void CMasternodeMetaInfo::RemoveGovernanceObject(const uint256& nGovernanceObjectHash)
{
    LOCK(cs);
    // Whether or not the govobj hash exists in the map first is irrelevant.
    mapGovernanceObjectsVotedOn.erase(nGovernanceObjectHash);
}

CMasternodeMetaInfoPtr CMasternodeMetaMan::GetMetaInfo(const uint256& proTxHash, bool fCreate) EXCLUSIVE_LOCKS_REQUIRED(!cs)
{
    LOCK(cs);
    auto it = metaInfos.find(proTxHash);
    if (it != metaInfos.end()) {
        return it->second;
    }
    if (!fCreate) {
        return nullptr;
    }
    it = metaInfos.emplace(proTxHash, std::make_shared<CMasternodeMetaInfo>(proTxHash)).first;
    return it->second;
}

// We keep track of dsq (mixing queues) count to avoid using same masternodes for mixing too often.
// This threshold is calculated as the last dsq count this specific masternode was used in a mixing
// session plus a margin of 20% of masternode count. In other words we expect at least 20% of unique
// masternodes before we ever see a masternode that we know already mixed someone's funds earlier.
int64_t CMasternodeMetaMan::GetDsqThreshold(const uint256& proTxHash, int nMnCount)
{
    auto metaInfo = GetMetaInfo(proTxHash);
    if (metaInfo == nullptr) {
        // return a threshold which is slightly above nDsqCount i.e. a no-go
        return nDsqCount + 1;
    }
    return metaInfo->GetLastDsq() + nMnCount / 5;
}

void CMasternodeMetaMan::AllowMixing(const uint256& proTxHash)
{
    auto mm = GetMetaInfo(proTxHash);
    nDsqCount++;
    mm->nLastDsq = nDsqCount.load();
    mm->nMixingTxCount = 0;
}

void CMasternodeMetaMan::DisallowMixing(const uint256& proTxHash)
{
    auto mm = GetMetaInfo(proTxHash);
    mm->nMixingTxCount++;
}

bool CMasternodeMetaMan::AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash)
{
    auto mm = GetMetaInfo(proTxHash);
    mm->AddGovernanceVote(nGovernanceObjectHash);
    return true;
}

void CMasternodeMetaMan::RemoveGovernanceObject(const uint256& nGovernanceObjectHash) EXCLUSIVE_LOCKS_REQUIRED(!cs)
{
    LOCK(cs);
    for(const auto& p : metaInfos) {
        p.second->RemoveGovernanceObject(nGovernanceObjectHash);
    }
}

std::vector<uint256> CMasternodeMetaMan::GetAndClearDirtyGovernanceObjectHashes() EXCLUSIVE_LOCKS_REQUIRED(!cs)
{
    std::vector<uint256> vecTmp;
    WITH_LOCK(cs, vecTmp.swap(vecDirtyGovernanceObjectHashes));
    return vecTmp;
}

bool CMasternodeMetaMan::AlreadyHavePlatformBan(const uint256& inv_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs)
{
    LOCK(cs);
    return m_seen_platform_bans.exists(inv_hash);
}

std::optional<PlatformBanMessage> CMasternodeMetaMan::GetPlatformBan(const uint256& inv_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs)
{
    LOCK(cs);
    PlatformBanMessage ret;
    if (!m_seen_platform_bans.get(inv_hash, ret)) {
        return std::nullopt;
    }

    return ret;
}

void CMasternodeMetaMan::RememberPlatformBan(const uint256& inv_hash, PlatformBanMessage&& msg) EXCLUSIVE_LOCKS_REQUIRED(!cs)
{
    LOCK(cs);
    m_seen_platform_bans.insert(inv_hash, std::move(msg));
}

std::string MasternodeMetaStore::ToString() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
{
    LOCK(cs);
    return strprintf("Masternodes: meta infos object count: %d, nDsqCount: %d", metaInfos.size(), nDsqCount);
}

uint256 PlatformBanMessage::GetHash() const { return ::SerializeHash(*this); }
