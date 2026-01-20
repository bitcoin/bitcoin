// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/meta.h>

#include <flat-database.h>
#include <univalue.h>
#include <util/time.h>

const std::string MasternodeMetaStore::SERIALIZATION_VERSION_STRING = "CMasternodeMetaMan-Version-5";

static constexpr int MASTERNODE_MAX_FAILED_OUTBOUND_ATTEMPTS{5};
static constexpr int MASTERNODE_MAX_MIXING_TXES{5};

namespace {
static const CMasternodeMetaInfo default_meta_info{};
} // anonymous namespace

CMasternodeMetaMan::CMasternodeMetaMan() :
    m_db{std::make_unique<db_type>("mncache.dat", "magicMasternodeCache")}
{
}

CMasternodeMetaMan::~CMasternodeMetaMan()
{
    if (!is_valid) return;
    m_db->Store(*this);
}

bool CMasternodeMetaMan::LoadCache(bool load_cache)
{
    assert(m_db != nullptr);
    is_valid = load_cache ? m_db->Load(*this) : m_db->Store(*this);
    return is_valid;
}

UniValue CMasternodeMetaInfo::ToJson() const
{
    int64_t now = GetTime<std::chrono::seconds>().count();

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("lastDSQ", m_last_dsq);
    ret.pushKV("mixingTxCount", m_mixing_tx_count);
    ret.pushKV("outboundAttemptCount", outboundAttemptCount);
    ret.pushKV("lastOutboundAttempt", lastOutboundAttempt);
    ret.pushKV("lastOutboundAttemptElapsed", now - lastOutboundAttempt);
    ret.pushKV("lastOutboundSuccess", lastOutboundSuccess);
    ret.pushKV("lastOutboundSuccessElapsed", now - lastOutboundSuccess);
    ret.pushKV("is_platform_banned", m_platform_ban);
    ret.pushKV("platform_ban_height_updated", m_platform_ban_updated);

    return ret;
}

void CMasternodeMetaInfo::AddGovernanceVote(const uint256& nGovernanceObjectHash)
{
    // Insert a zero value, or not. Then increment the value regardless. This
    // ensures the value is in the map.
    const auto& pair = mapGovernanceObjectsVotedOn.emplace(nGovernanceObjectHash, 0);
    pair.first->second++;
}

void CMasternodeMetaInfo::RemoveGovernanceObject(const uint256& nGovernanceObjectHash)
{
    // Whether or not the govobj hash exists in the map first is irrelevant.
    mapGovernanceObjectsVotedOn.erase(nGovernanceObjectHash);
}

const CMasternodeMetaInfo& CMasternodeMetaMan::GetMetaInfoOrDefault(const uint256& protx_hash) const
{
    const auto it = metaInfos.find(protx_hash);
    if (it == metaInfos.end()) return default_meta_info;
    return it->second;
}

CMasternodeMetaInfo CMasternodeMetaMan::GetInfo(const uint256& proTxHash) const
{
    LOCK(cs);
    return GetMetaInfoOrDefault(proTxHash);
}

CMasternodeMetaInfo& CMasternodeMetaMan::GetMetaInfo(const uint256& proTxHash)
{
    auto it = metaInfos.find(proTxHash);
    if (it != metaInfos.end()) {
        return it->second;
    }
    it = metaInfos.emplace(proTxHash, CMasternodeMetaInfo{proTxHash}).first;
    return it->second;
}

bool CMasternodeMetaMan::IsMixingThresholdExceeded(const uint256& protx_hash, int mn_count) const
{
    LOCK(cs);
    auto it = metaInfos.find(protx_hash);
    if (it == metaInfos.end()) {
        LogPrint(BCLog::COINJOIN, "DSQUEUE -- node %s is logged\n", protx_hash.ToString());
        return false;
    }
    const auto& meta_info = it->second;
    int64_t last_dsq = meta_info.m_last_dsq;
    int64_t threshold = last_dsq + mn_count / 5;

    LogPrint(BCLog::COINJOIN, "DSQUEUE -- mn: %s last_dsq: %d  dsq_threshold: %d  nDsqCount: %d\n",
             protx_hash.ToString(), last_dsq, threshold, nDsqCount);
    return last_dsq != 0 && threshold > nDsqCount;
}

void CMasternodeMetaMan::AllowMixing(const uint256& proTxHash)
{
    LOCK(cs);
    auto& mm = GetMetaInfo(proTxHash);
    mm.m_last_dsq = ++nDsqCount;
    mm.m_mixing_tx_count = 0;
}

void CMasternodeMetaMan::DisallowMixing(const uint256& proTxHash)
{
    LOCK(cs);
    GetMetaInfo(proTxHash).m_mixing_tx_count++;
}

bool CMasternodeMetaMan::IsValidForMixingTxes(const uint256& protx_hash) const
{
    LOCK(cs);
    return GetMetaInfoOrDefault(protx_hash).m_mixing_tx_count <= MASTERNODE_MAX_MIXING_TXES;
}

void CMasternodeMetaMan::AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash)
{
    LOCK(cs);
    GetMetaInfo(proTxHash).AddGovernanceVote(nGovernanceObjectHash);
}

void CMasternodeMetaMan::RemoveGovernanceObject(const uint256& nGovernanceObjectHash)
{
    LOCK(cs);
    for (auto& [_, meta_info] : metaInfos) {
        meta_info.RemoveGovernanceObject(nGovernanceObjectHash);
    }
}

std::vector<uint256> CMasternodeMetaMan::GetAndClearDirtyGovernanceObjectHashes()
{
    std::vector<uint256> vecTmp;
    WITH_LOCK(cs, vecTmp.swap(vecDirtyGovernanceObjectHashes));
    return vecTmp;
}

void CMasternodeMetaMan::SetLastOutboundAttempt(const uint256& protx_hash, int64_t t)
{
    LOCK(cs);
    GetMetaInfo(protx_hash).SetLastOutboundAttempt(t);
}

void CMasternodeMetaMan::SetLastOutboundSuccess(const uint256& protx_hash, int64_t t)
{
    LOCK(cs);
    GetMetaInfo(protx_hash).SetLastOutboundSuccess(t);
}

int64_t CMasternodeMetaMan::GetLastOutboundAttempt(const uint256& protx_hash) const
{
    LOCK(cs);
    return GetMetaInfoOrDefault(protx_hash).lastOutboundAttempt;
}

int64_t CMasternodeMetaMan::GetLastOutboundSuccess(const uint256& protx_hash) const
{
    LOCK(cs);
    return GetMetaInfoOrDefault(protx_hash).lastOutboundSuccess;
}

bool CMasternodeMetaMan::OutboundFailedTooManyTimes(const uint256& protx_hash) const
{
    LOCK(cs);
    return GetMetaInfoOrDefault(protx_hash).outboundAttemptCount > MASTERNODE_MAX_FAILED_OUTBOUND_ATTEMPTS;
}

bool CMasternodeMetaMan::IsPlatformBanned(const uint256& protx_hash) const
{
    LOCK(cs);
    return GetMetaInfoOrDefault(protx_hash).m_platform_ban;
}

bool CMasternodeMetaMan::ResetPlatformBan(const uint256& protx_hash, int height)
{
    LOCK(cs);

    auto it = metaInfos.find(protx_hash);
    if (it == metaInfos.end()) return false;

    return it->second.SetPlatformBan(false, height);
}

bool CMasternodeMetaMan::SetPlatformBan(const uint256& inv_hash, PlatformBanMessage&& ban_msg)
{
    LOCK(cs);

    const uint256& protx_hash = ban_msg.m_protx_hash;

    bool ret = GetMetaInfo(protx_hash).SetPlatformBan(true, ban_msg.m_requested_height);
    if (ret) {
        m_seen_platform_bans.emplace(inv_hash, std::move(ban_msg));
    }
    return ret;
}

bool CMasternodeMetaMan::AlreadyHavePlatformBan(const uint256& inv_hash) const
{
    LOCK(cs);
    return m_seen_platform_bans.exists(inv_hash);
}

std::optional<PlatformBanMessage> CMasternodeMetaMan::GetPlatformBan(const uint256& inv_hash) const
{
    LOCK(cs);
    PlatformBanMessage ret;
    if (!m_seen_platform_bans.get(inv_hash, ret)) {
        return std::nullopt;
    }

    return ret;
}

void CMasternodeMetaMan::AddUsedMasternode(const uint256& proTxHash)
{
    LOCK(cs);
    // Only add if not already present (prevents duplicates)
    if (m_used_masternodes_set.insert(proTxHash).second) {
        m_used_masternodes.push_back(proTxHash);
    }
}

void CMasternodeMetaMan::RemoveUsedMasternodes(size_t count)
{
    LOCK(cs);
    size_t removed = 0;
    while (removed < count && !m_used_masternodes.empty()) {
        // Remove from both the set and the deque
        m_used_masternodes_set.erase(m_used_masternodes.front());
        m_used_masternodes.pop_front();
        ++removed;
    }
}

size_t CMasternodeMetaMan::GetUsedMasternodesCount() const
{
    LOCK(cs);
    return m_used_masternodes.size();
}

bool CMasternodeMetaMan::IsUsedMasternode(const uint256& proTxHash) const
{
    LOCK(cs);
    return m_used_masternodes_set.find(proTxHash) != m_used_masternodes_set.end();
}

std::string MasternodeMetaStore::ToString() const
{
    LOCK(cs);
    return strprintf("Masternodes: meta infos object count: %d, nDsqCount: %d, used masternodes count: %d",
                     metaInfos.size(), nDsqCount, m_used_masternodes.size());
}

uint256 PlatformBanMessage::GetHash() const { return ::SerializeHash(*this); }
