// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_MASTERNODE_MASTERNODEMETA_H
#define SYSCOIN_MASTERNODE_MASTERNODEMETA_H

#include <serialize.h>

#include <univalue.h>

#include <atomic>
#include <uint256.h>
#include <sync.h>

class CConnman;

static constexpr int MASTERNODE_MAX_FAILED_OUTBOUND_ATTEMPTS{5};

// Holds extra (non-deterministic) information about masternodes
// This is mostly local information, e.g. about mixing and governance
class CMasternodeMetaInfo
{
    friend class CMasternodeMetaMan;

private:
    mutable RecursiveMutex cs;

    uint256 proTxHash GUARDED_BY(cs);

    // KEEP TRACK OF GOVERNANCE ITEMS EACH MASTERNODE HAS VOTE UPON FOR RECALCULATION
    std::map<uint256, int> mapGovernanceObjectsVotedOn GUARDED_BY(cs);

    std::atomic<int> outboundAttemptCount{0};
    std::atomic<int64_t> lastOutboundAttempt{0};
    std::atomic<int64_t> lastOutboundSuccess{0};

public:
    CMasternodeMetaInfo() = default;
    explicit CMasternodeMetaInfo(const uint256& _proTxHash) : proTxHash(_proTxHash) {}
    CMasternodeMetaInfo(const CMasternodeMetaInfo& ref) :
        proTxHash(ref.proTxHash),
        mapGovernanceObjectsVotedOn(ref.mapGovernanceObjectsVotedOn),
        lastOutboundAttempt(ref.lastOutboundAttempt.load()),
        lastOutboundSuccess(ref.lastOutboundSuccess.load())
    {
    }
    SERIALIZE_METHODS(CMasternodeMetaInfo, obj) {
        LOCK(obj.cs);
        READWRITE(obj.proTxHash, obj.mapGovernanceObjectsVotedOn, obj.outboundAttemptCount, obj.lastOutboundAttempt, obj.lastOutboundSuccess);
    }

    UniValue ToJson() const;

public:
    const uint256& GetProTxHash() const { LOCK(cs); return proTxHash; }

    // KEEP TRACK OF EACH GOVERNANCE ITEM IN CASE THIS NODE GOES OFFLINE, SO WE CAN RECALCULATE THEIR STATUS
    void AddGovernanceVote(const uint256& nGovernanceObjectHash);

    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);
    bool OutboundFailedTooManyTimes() const { return outboundAttemptCount > MASTERNODE_MAX_FAILED_OUTBOUND_ATTEMPTS; }
    void SetLastOutboundAttempt(int64_t t) { lastOutboundAttempt = t; ++outboundAttemptCount; }
    int64_t GetLastOutboundAttempt() const { return lastOutboundAttempt; }
    void SetLastOutboundSuccess(int64_t t) { lastOutboundSuccess = t; outboundAttemptCount = 0; }
    int64_t GetLastOutboundSuccess() const { return lastOutboundSuccess; }
};
using CMasternodeMetaInfoPtr = std::shared_ptr<CMasternodeMetaInfo>;

class CMasternodeMetaMan
{
private:
    static const std::string SERIALIZATION_VERSION_STRING;

    mutable RecursiveMutex cs;

    std::map<uint256, CMasternodeMetaInfoPtr> metaInfos GUARDED_BY(cs);
    std::vector<uint256> vecDirtyGovernanceObjectHashes GUARDED_BY(cs);

public:
    template<typename Stream>
    void Serialize(Stream& s) const
    {
        LOCK(cs);
        std::string strVersion;
        strVersion = SERIALIZATION_VERSION_STRING;
        s << strVersion;
        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        for (auto& p : metaInfos) {
            tmpMetaInfo.emplace_back(*p.second);
        }
        s << tmpMetaInfo;
    }
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        LOCK(cs);
        
        std::string strVersion;
        Clear();
        s >> strVersion;
        if (strVersion != SERIALIZATION_VERSION_STRING)
            return;
        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        s >> tmpMetaInfo;
        metaInfos.clear();
        for (auto& mm : tmpMetaInfo) {
            const auto& mmHash = mm.GetProTxHash();
            metaInfos.emplace(mmHash, std::make_shared<CMasternodeMetaInfo>(std::move(mm)));
        }
    }

public:
    CMasternodeMetaInfoPtr GetMetaInfo(const uint256& proTxHash, bool fCreate = true);

    bool AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);

    std::vector<uint256> GetAndClearDirtyGovernanceObjectHashes();

    void Clear();
    // Needed to avoid errors in flat-database.h
    void CheckAndRemove() const {};

    std::string ToString() const;
};

extern CMasternodeMetaMan mmetaman;

#endif // SYSCOIN_MASTERNODE_MASTERNODEMETA_H
