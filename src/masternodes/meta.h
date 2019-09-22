// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_MASTERNODE_META_H
#define BITGREEN_MASTERNODE_META_H

#include <serialize.h>
#include <special/deterministicmns.h>

#include <memory>

class CConnman;

static const int MASTERNODE_MAX_MIXING_TXES             = 5;

// Holds extra (non-deterministic) information about masternodes
// This is mostly local information, e.g. about mixing and governance
class CMasternodeMetaInfo
{
    friend class CMasternodeMetaMan;

private:
    mutable CCriticalSection cs;

    uint256 proTxHash;

    //the dsq count from the last dsq broadcast of this node
    int64_t nLastDsq = 0;
    int nMixingTxCount = 0;

    // KEEP TRACK OF GOVERNANCE ITEMS EACH MASTERNODE HAS VOTE UPON FOR RECALCULATION
    std::map<uint256, int> mapGovernanceObjectsVotedOn;

public:
    CMasternodeMetaInfo() {}
    CMasternodeMetaInfo(const uint256& _proTxHash) : proTxHash(_proTxHash) {}
    CMasternodeMetaInfo(const CMasternodeMetaInfo& ref) :
        proTxHash(ref.proTxHash),
        nLastDsq(ref.nLastDsq),
        nMixingTxCount(ref.nMixingTxCount),
        mapGovernanceObjectsVotedOn(ref.mapGovernanceObjectsVotedOn)
    {
    }

    template <typename Stream>
    CMasternodeMetaInfo(deserialize_type, Stream& s)
    {
        s >> *this;
    }

    ADD_SERIALIZE_METHODS
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        LOCK(cs);
        READWRITE(proTxHash);
        READWRITE(nLastDsq);
        READWRITE(nMixingTxCount);
        READWRITE(mapGovernanceObjectsVotedOn);
    }

public:
    const uint256& GetProTxHash() const { LOCK(cs); return proTxHash; }
    int64_t GetLastDsq() const { LOCK(cs); return nLastDsq; }
    int GetMixingTxCount() const { LOCK(cs); return nMixingTxCount; }

    bool IsValidForMixingTxes() const { return GetMixingTxCount() <= MASTERNODE_MAX_MIXING_TXES; }

    // KEEP TRACK OF EACH GOVERNANCE ITEM INCASE THIS NODE GOES OFFLINE, SO WE CAN RECALC THEIR STATUS
    void AddGovernanceVote(const uint256& nGovernanceObjectHash);
    // RECALCULATE CACHED STATUS FLAGS FOR ALL AFFECTED OBJECTS
    void FlagGovernanceItemsAsDirty();

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

    // keep track of dsq count to prevent masternodes from gaming privatesend queue
    int64_t nDsqCount = 0;

public:
    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        LOCK(cs);

        std::string strVersion;
        if(ser_action.ForRead()) {
            Clear();
            READWRITE(strVersion);
            if (strVersion != SERIALIZATION_VERSION_STRING) {
                return;
            }
        }
        else {
            strVersion = SERIALIZATION_VERSION_STRING;
            READWRITE(strVersion);
        }

        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        if (ser_action.ForRead()) {
            READWRITE(tmpMetaInfo);
            metaInfos.clear();
            for (auto& mm : tmpMetaInfo) {
                metaInfos.emplace(mm.GetProTxHash(), std::make_shared<CMasternodeMetaInfo>(std::move(mm)));
            }
        } else {
            for (auto& p : metaInfos) {
                tmpMetaInfo.emplace_back(*p.second);
            }
            READWRITE(tmpMetaInfo);
        }

        READWRITE(nDsqCount);
    }

public:
    CMasternodeMetaInfoPtr GetMetaInfo(const uint256& proTxHash, bool fCreate = true);

    bool AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);

    void AddDirtyGovernanceObjectHash(const uint256& nHash);
    std::vector<uint256> GetAndClearDirtyGovernanceObjectHashes();

    void Clear();
    void CheckAndRemove();

    std::string ToString() const;
};

extern CMasternodeMetaMan mmetaman;

#endif//BITGREEN_MASTERNODE_META_H
