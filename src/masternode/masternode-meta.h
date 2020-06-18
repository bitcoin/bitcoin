// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_MASTERNODE_MASTERNODE_META_H
#define SYSCOIN_MASTERNODE_MASTERNODE_META_H

#include <serialize.h>

#include <evo/deterministicmns.h>

#include <memory>

class CConnman;


// Holds extra (non-deterministic) information about masternodes
// This is mostly local information, e.g. about governance
class CMasternodeMetaInfo
{
    friend class CMasternodeMetaMan;

private:
    mutable CCriticalSection cs;

    uint256 proTxHash;

    // KEEP TRACK OF GOVERNANCE ITEMS EACH MASTERNODE HAS VOTE UPON FOR RECALCULATION
    std::map<uint256, int> mapGovernanceObjectsVotedOn;

public:
    CMasternodeMetaInfo() {}
    CMasternodeMetaInfo(const uint256& _proTxHash) : proTxHash(_proTxHash) {}
    CMasternodeMetaInfo(const CMasternodeMetaInfo& ref) :
        proTxHash(ref.proTxHash),
        mapGovernanceObjectsVotedOn(ref.mapGovernanceObjectsVotedOn)
    {
    }

    SERIALIZE_METHODS(CMasternodeMetaInfo, obj)
    {
        LOCK(cs);
        READWRITE(obj.proTxHash, obj.mapGovernanceObjectsVotedOn);
    }

public:
    const uint256& GetProTxHash() const { LOCK(cs); return proTxHash; }

    // KEEP TRACK OF EACH GOVERNANCE ITEM INCASE THIS NODE GOES OFFLINE, SO WE CAN RECALC THEIR STATUS
    void AddGovernanceVote(const uint256& nGovernanceObjectHash);

    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);
};
typedef std::shared_ptr<CMasternodeMetaInfo> CMasternodeMetaInfoPtr;

class CMasternodeMetaMan
{
private:
    static const std::string SERIALIZATION_VERSION_STRING;

    CCriticalSection cs;

    std::map<uint256, CMasternodeMetaInfoPtr> metaInfos;
    std::vector<uint256> vecDirtyGovernanceObjectHashes;

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
        if (strVersion != SERIALIZATION_VERSION_STRING) {
            return;
        }
        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        s >> tmpMetaInfo;
        metaInfos.clear();
        for (auto& mm : tmpMetaInfo) {
            metaInfos.emplace(mm.GetProTxHash(), std::make_shared<CMasternodeMetaInfo>(std::move(mm)));
        }
    }

public:
    CMasternodeMetaInfoPtr GetMetaInfo(const uint256& proTxHash, bool fCreate = true);

    bool AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);

    std::vector<uint256> GetAndClearDirtyGovernanceObjectHashes();

    void Clear();
    void CheckAndRemove();

    std::string ToString() const;
};

extern CMasternodeMetaMan mmetaman;

#endif//MASTERNODE_META_H
