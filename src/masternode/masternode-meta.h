// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_MASTERNODE_META_H
#define BITCOIN_MASTERNODE_MASTERNODE_META_H

#include <serialize.h>

#include <univalue.h>

#include <uint256.h>
#include <sync.h>

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

    int64_t lastOutboundAttempt = 0;
    int64_t lastOutboundSuccess = 0;

public:
    CMasternodeMetaInfo() = default;
    explicit CMasternodeMetaInfo(const uint256& _proTxHash) : proTxHash(_proTxHash) {}
    CMasternodeMetaInfo(const CMasternodeMetaInfo& ref) :
        proTxHash(ref.proTxHash),
        nLastDsq(ref.nLastDsq),
        nMixingTxCount(ref.nMixingTxCount),
        mapGovernanceObjectsVotedOn(ref.mapGovernanceObjectsVotedOn),
        lastOutboundAttempt(ref.lastOutboundAttempt),
        lastOutboundSuccess(ref.lastOutboundSuccess)
    {
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
        READWRITE(lastOutboundAttempt);
        READWRITE(lastOutboundSuccess);
    }

    UniValue ToJson() const;

public:
    const uint256& GetProTxHash() const { LOCK(cs); return proTxHash; }
    int64_t GetLastDsq() const { LOCK(cs); return nLastDsq; }
    int GetMixingTxCount() const { LOCK(cs); return nMixingTxCount; }

    bool IsValidForMixingTxes() const { return GetMixingTxCount() <= MASTERNODE_MAX_MIXING_TXES; }

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

    CCriticalSection cs;

    std::map<uint256, CMasternodeMetaInfoPtr> metaInfos;
    std::vector<uint256> vecDirtyGovernanceObjectHashes;

    // keep track of dsq count to prevent masternodes from gaming coinjoin queue
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

    int64_t GetDsqCount() { LOCK(cs); return nDsqCount; }
    int64_t GetDsqThreshold(const uint256& proTxHash, int nMnCount);

    void AllowMixing(const uint256& proTxHash);
    void DisallowMixing(const uint256& proTxHash);

    bool AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);

    std::vector<uint256> GetAndClearDirtyGovernanceObjectHashes();

    void Clear();
    void CheckAndRemove();

    std::string ToString() const;
};

extern CMasternodeMetaMan mmetaman;

#endif // BITCOIN_MASTERNODE_MASTERNODE_META_H
