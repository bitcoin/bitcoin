// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_META_H
#define BITCOIN_MASTERNODE_META_H

#include <serialize.h>
#include <sync.h>
#include <uint256.h>

#include <univalue.h>

#include <atomic>
#include <memory>

class CConnman;
template<typename T>
class CFlatDB;

static constexpr int MASTERNODE_MAX_MIXING_TXES{5};
static constexpr int MASTERNODE_MAX_FAILED_OUTBOUND_ATTEMPTS{5};

// Holds extra (non-deterministic) information about masternodes
// This is mostly local information, e.g. about mixing and governance
class CMasternodeMetaInfo
{
    friend class CMasternodeMetaMan;

private:
    mutable RecursiveMutex cs;

    uint256 proTxHash GUARDED_BY(cs);

    //the dsq count from the last dsq broadcast of this node
    std::atomic<int64_t> nLastDsq{0};
    std::atomic<int> nMixingTxCount{0};

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
        nLastDsq(ref.nLastDsq.load()),
        nMixingTxCount(ref.nMixingTxCount.load()),
        mapGovernanceObjectsVotedOn(ref.mapGovernanceObjectsVotedOn),
        lastOutboundAttempt(ref.lastOutboundAttempt.load()),
        lastOutboundSuccess(ref.lastOutboundSuccess.load())
    {
    }

    SERIALIZE_METHODS(CMasternodeMetaInfo, obj)
    {
        LOCK(obj.cs);
        READWRITE(
                obj.proTxHash,
                obj.nLastDsq,
                obj.nMixingTxCount,
                obj.mapGovernanceObjectsVotedOn,
                obj.outboundAttemptCount,
                obj.lastOutboundAttempt,
                obj.lastOutboundSuccess
                );
    }

    UniValue ToJson() const;

public:
    const uint256& GetProTxHash() const { LOCK(cs); return proTxHash; }
    int64_t GetLastDsq() const { return nLastDsq; }
    int GetMixingTxCount() const { return nMixingTxCount; }

    bool IsValidForMixingTxes() const { return GetMixingTxCount() <= MASTERNODE_MAX_MIXING_TXES; }

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

class MasternodeMetaStore
{
protected:
    static const std::string SERIALIZATION_VERSION_STRING;

    mutable RecursiveMutex cs;
    std::map<uint256, CMasternodeMetaInfoPtr> metaInfos GUARDED_BY(cs);
    // keep track of dsq count to prevent masternodes from gaming coinjoin queue
    std::atomic<int64_t> nDsqCount{0};

public:
    template<typename Stream>
    void Serialize(Stream &s) const
    {
        LOCK(cs);
        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        for (const auto& p : metaInfos) {
            tmpMetaInfo.emplace_back(*p.second);
        }
        s << SERIALIZATION_VERSION_STRING << tmpMetaInfo << nDsqCount;
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        Clear();

        LOCK(cs);
        std::string strVersion;
        s >> strVersion;
        if (strVersion != SERIALIZATION_VERSION_STRING) {
            return;
        }
        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        s >> tmpMetaInfo >> nDsqCount;
        metaInfos.clear();
        for (auto& mm : tmpMetaInfo) {
            metaInfos.emplace(mm.GetProTxHash(), std::make_shared<CMasternodeMetaInfo>(std::move(mm)));
        }
    }

    void Clear()
    {
        LOCK(cs);

        metaInfos.clear();
    }

    std::string ToString() const;
};

class CMasternodeMetaMan : public MasternodeMetaStore
{
private:
    using db_type = CFlatDB<MasternodeMetaStore>;

private:
    const std::unique_ptr<db_type> m_db;
    const bool is_valid{false};

    std::vector<uint256> vecDirtyGovernanceObjectHashes GUARDED_BY(cs);

public:
    explicit CMasternodeMetaMan(bool load_cache);
    ~CMasternodeMetaMan();

    bool IsValid() const { return is_valid; }

    CMasternodeMetaInfoPtr GetMetaInfo(const uint256& proTxHash, bool fCreate = true);

    int64_t GetDsqCount() const { return nDsqCount; }
    int64_t GetDsqThreshold(const uint256& proTxHash, int nMnCount);

    void AllowMixing(const uint256& proTxHash);
    void DisallowMixing(const uint256& proTxHash);

    bool AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);

    std::vector<uint256> GetAndClearDirtyGovernanceObjectHashes();
};

extern std::unique_ptr<CMasternodeMetaMan> mmetaman;

#endif // BITCOIN_MASTERNODE_META_H
