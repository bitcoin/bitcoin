// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/meta.h>

#include <flat-database.h>
#include <util/time.h>

#include <sstream>

std::unique_ptr<CMasternodeMetaMan> mmetaman;

const std::string MasternodeMetaStore::SERIALIZATION_VERSION_STRING = "CMasternodeMetaMan-Version-3";

CMasternodeMetaMan::CMasternodeMetaMan(bool load_cache) :
    m_db{std::make_unique<db_type>("mncache.dat", "magicMasternodeCache")},
    is_valid{
        [&]() -> bool {
            assert(m_db != nullptr);
            return load_cache ? m_db->Load(*this) : m_db->Store(*this);
        }()
    }
{
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

    ret.pushKV("lastDSQ", nLastDsq);
    ret.pushKV("mixingTxCount", nMixingTxCount);
    ret.pushKV("outboundAttemptCount", outboundAttemptCount);
    ret.pushKV("lastOutboundAttempt", lastOutboundAttempt);
    ret.pushKV("lastOutboundAttemptElapsed", now - lastOutboundAttempt);
    ret.pushKV("lastOutboundSuccess", lastOutboundSuccess);
    ret.pushKV("lastOutboundSuccessElapsed", now - lastOutboundSuccess);

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

CMasternodeMetaInfoPtr CMasternodeMetaMan::GetMetaInfo(const uint256& proTxHash, bool fCreate)
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

void CMasternodeMetaMan::RemoveGovernanceObject(const uint256& nGovernanceObjectHash)
{
    LOCK(cs);
    for(const auto& p : metaInfos) {
        p.second->RemoveGovernanceObject(nGovernanceObjectHash);
    }
}

std::vector<uint256> CMasternodeMetaMan::GetAndClearDirtyGovernanceObjectHashes()
{
    LOCK(cs);
    std::vector<uint256> vecTmp = std::move(vecDirtyGovernanceObjectHashes);
    vecDirtyGovernanceObjectHashes.clear();
    return vecTmp;
}

std::string MasternodeMetaStore::ToString() const
{
    std::ostringstream info;
    LOCK(cs);
    info << "Masternodes: meta infos object count: " << (int)metaInfos.size() <<
         ", nDsqCount: " << (int)nDsqCount;
    return info.str();
}
