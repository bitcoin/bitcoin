// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_MASTERNODE_MASTERNODEMETA_H
#define SYSCOIN_MASTERNODE_MASTERNODEMETA_H

#include <serialize.h>

#include <univalue.h>

#include <uint256.h>
#include <sync.h>
#include <version.h>
#include <util/time.h>
class CConnman;


// Holds extra (non-deterministic) information about masternodes
// This is mostly local information, e.g. about mixing and governance
class CMasternodeMetaInfo
{
    friend class CMasternodeMetaMan;

private:
    mutable RecursiveMutex cs;

    uint256 proTxHash;

    // KEEP TRACK OF GOVERNANCE ITEMS EACH MASTERNODE HAS VOTE UPON FOR RECALCULATION
    std::map<uint256, int> mapGovernanceObjectsVotedOn;

    int64_t lastOutboundAttempt = 0;
    int64_t lastOutboundSuccess = 0;

public:
    CMasternodeMetaInfo() = default;
    explicit CMasternodeMetaInfo(const uint256& _proTxHash) : proTxHash(_proTxHash) {}
    CMasternodeMetaInfo(const CMasternodeMetaInfo& ref) :
        proTxHash(ref.proTxHash),
        mapGovernanceObjectsVotedOn(ref.mapGovernanceObjectsVotedOn),
        lastOutboundAttempt(ref.lastOutboundAttempt),
        lastOutboundSuccess(ref.lastOutboundSuccess)
    {
    }
    SERIALIZE_METHODS(CMasternodeMetaInfo, obj) {
        LOCK(obj.cs);
        READWRITE(obj.proTxHash, obj.mapGovernanceObjectsVotedOn, obj.lastOutboundAttempt, obj.lastOutboundSuccess);
    }

    UniValue ToJson() const;

public:
    const uint256& GetProTxHash() const { LOCK(cs); return proTxHash; }

    // KEEP TRACK OF EACH GOVERNANCE ITEM IN CASE THIS NODE GOES OFFLINE, SO WE CAN RECALCULATE THEIR STATUS
    void AddGovernanceVote(const uint256& nGovernanceObjectHash);

    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);

    void SetLastOutboundAttempt(int64_t t) { LOCK(cs); lastOutboundAttempt = t; }
    int64_t GetLastOutboundAttempt() const { LOCK(cs); return lastOutboundAttempt; }
    void SetLastOutboundSuccess(int64_t t) { LOCK(cs); lastOutboundSuccess = t; }
    int64_t GetLastOutboundSuccess() const { LOCK(cs); return lastOutboundSuccess; }
};
typedef std::shared_ptr<CMasternodeMetaInfo> CMasternodeMetaInfoPtr;

class CMasternodeMetaMan
{
private:
    static const std::string SERIALIZATION_VERSION_STRING;

    mutable RecursiveMutex cs;

    std::map<uint256, CMasternodeMetaInfoPtr> metaInfos;
    std::vector<uint256> vecDirtyGovernanceObjectHashes;
    int nCurrentVersion = 0;
    int64_t nCurrentVersionStarted = 0;
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
        if (nCurrentVersion == 0 && nCurrentVersionStarted == 0) {
            // first time serialization
            s << MIN_MASTERNODE_PROTO_VERSION;
            s << GetTime();
        } else {
            s << nCurrentVersion;
            s << nCurrentVersionStarted;
        }
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
            metaInfos.try_emplace(mm.GetProTxHash(), std::make_shared<CMasternodeMetaInfo>(std::move(mm)));
        }
        s >> nCurrentVersion;
        s >> nCurrentVersionStarted;
    }

public:
    CMasternodeMetaInfoPtr GetMetaInfo(const uint256& proTxHash, bool fCreate = true);

    bool AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);
    int64_t GetCurrentVersionStarted() { LOCK(cs); return nCurrentVersionStarted; }
    std::vector<uint256> GetAndClearDirtyGovernanceObjectHashes();

    void Clear();
    void CheckAndRemove();

    std::string ToString() const;
};

extern CMasternodeMetaMan mmetaman;

#endif // SYSCOIN_MASTERNODE_MASTERNODEMETA_H
