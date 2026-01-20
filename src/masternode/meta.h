// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_META_H
#define BITCOIN_MASTERNODE_META_H

#include <bls/bls.h>
#include <saltedhasher.h>
#include <serialize.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <unordered_lru_cache.h>

#include <deque>
#include <map>
#include <optional>
#include <vector>

class UniValue;

template<typename T>
class CFlatDB;

// Holds extra (non-deterministic) information about masternodes
// This is mostly local information, e.g. about mixing and governance
class CMasternodeMetaInfo
{
public:
    uint256 m_protx_hash;

    //! the dsq count from the last dsq broadcast of this node
    int64_t m_last_dsq{0};
    int m_mixing_tx_count{0};

    // KEEP TRACK OF GOVERNANCE ITEMS EACH MASTERNODE HAS VOTE UPON FOR RECALCULATION
    std::map<uint256, int> mapGovernanceObjectsVotedOn;

    int outboundAttemptCount{0};
    int64_t lastOutboundAttempt{0};
    int64_t lastOutboundSuccess{0};

    //! bool flag is node currently under platform ban by p2p message
    bool m_platform_ban{false};
    //! height at which platform ban has been applied or removed
    int m_platform_ban_updated{0};

public:
    CMasternodeMetaInfo() = default;
    explicit CMasternodeMetaInfo(const uint256& protx_hash) :
        m_protx_hash(protx_hash)
    {
    }
    CMasternodeMetaInfo(const CMasternodeMetaInfo& ref) = default;

    SERIALIZE_METHODS(CMasternodeMetaInfo, obj)
    {
        READWRITE(obj.m_protx_hash, obj.m_last_dsq, obj.m_mixing_tx_count, obj.mapGovernanceObjectsVotedOn,
                  obj.outboundAttemptCount, obj.lastOutboundAttempt, obj.lastOutboundSuccess, obj.m_platform_ban,
                  obj.m_platform_ban_updated);
    }

    UniValue ToJson() const;

    // KEEP TRACK OF EACH GOVERNANCE ITEM IN CASE THIS NODE GOES OFFLINE, SO WE CAN RECALCULATE THEIR STATUS
    void AddGovernanceVote(const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash);

    void SetLastOutboundAttempt(int64_t t) { lastOutboundAttempt = t; ++outboundAttemptCount; }
    void SetLastOutboundSuccess(int64_t t) { lastOutboundSuccess = t; outboundAttemptCount = 0; }

    bool SetPlatformBan(bool is_banned, int height)
    {
        if (height < m_platform_ban_updated) {
            return false;
        }
        if (height == m_platform_ban_updated && !is_banned) {
            return false;
        }
        m_platform_ban = is_banned;
        m_platform_ban_updated = height;
        return true;
    }
};

class MasternodeMetaStore
{
protected:
    static const std::string SERIALIZATION_VERSION_STRING;

    mutable Mutex cs;
    std::map<uint256, CMasternodeMetaInfo> metaInfos GUARDED_BY(cs);
    // keep track of dsq count to prevent masternodes from gaming coinjoin queue
    int64_t nDsqCount GUARDED_BY(cs){0};
    // keep track of the used Masternodes for CoinJoin across all wallets
    // Using deque for efficient FIFO removal and unordered_set for O(1) lookups
    std::deque<uint256> m_used_masternodes GUARDED_BY(cs);
    Uint256HashSet m_used_masternodes_set GUARDED_BY(cs);

public:
    template<typename Stream>
    void Serialize(Stream &s) const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        for (const auto& p : metaInfos) {
            tmpMetaInfo.emplace_back(p.second);
        }
        // Convert deque to vector for serialization - unordered_set will be rebuilt on deserialization
        std::vector<uint256> tmpUsedMasternodes(m_used_masternodes.begin(), m_used_masternodes.end());
        s << SERIALIZATION_VERSION_STRING << tmpMetaInfo << nDsqCount << tmpUsedMasternodes;
    }

    template<typename Stream>
    void Unserialize(Stream &s) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);

        metaInfos.clear();
        std::string strVersion;
        s >> strVersion;
        if (strVersion != SERIALIZATION_VERSION_STRING) {
            return;
        }
        std::vector<CMasternodeMetaInfo> tmpMetaInfo;
        std::vector<uint256> tmpUsedMasternodes;
        s >> tmpMetaInfo >> nDsqCount >> tmpUsedMasternodes;
        for (auto& mm : tmpMetaInfo) {
            metaInfos.emplace(mm.m_protx_hash, CMasternodeMetaInfo{mm});
        }

        // Convert vector to deque and build unordered_set for O(1) lookups
        m_used_masternodes.assign(tmpUsedMasternodes.begin(), tmpUsedMasternodes.end());
        m_used_masternodes_set.clear();
        m_used_masternodes_set.insert(tmpUsedMasternodes.begin(), tmpUsedMasternodes.end());
    }

    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);

        metaInfos.clear();
        m_used_masternodes.clear();
        m_used_masternodes_set.clear();
    }

    std::string ToString() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

/**
 * Platform PoSe Ban are result in the node voting against the targeted evonode in all future DKG sessions until that targeted
 *evonode has been successfully banned. Platform will initiate this ban process by passing relevant information to Core using RPC. See DIP-0031
 *
 * We use 2 main classes to manage Platform PoSe Ban
 *
 * PlatformBanMessage
 * CMasternodeMetaInfo - a higher-level construct which store extra (non-deterministic) information about masternodes including platform ban status
 **/

/**
 * PlatformBanMessage - low-level constructs which present p2p message PlatformBan containing the protx hash, requested
 * height to ban, and BLS data: quorum hash and bls signature
 */
class PlatformBanMessage
{
public:
    uint256 m_protx_hash;
    int32_t m_requested_height{0};
    uint256 m_quorum_hash;
    CBLSSignature m_signature;

    PlatformBanMessage() = default;

    SERIALIZE_METHODS(PlatformBanMessage, obj)
    {
        READWRITE(obj.m_protx_hash, obj.m_requested_height, obj.m_quorum_hash);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.m_signature), false));
        }
    }

    uint256 GetHash() const;
};

class CMasternodeMetaMan : public MasternodeMetaStore
{
private:
    using db_type = CFlatDB<MasternodeMetaStore>;

private:
    const std::unique_ptr<db_type> m_db;
    bool is_valid{false};

    std::vector<uint256> vecDirtyGovernanceObjectHashes GUARDED_BY(cs);

    // equal to double of expected amount of all evo nodes, see DIP-0028
    // it consumes no more than 1Mb of RAM but will cover extreme cases
    static constexpr size_t SeenBanInventorySize = 900;
    mutable unordered_lru_cache<uint256, PlatformBanMessage, StaticSaltedHasher> m_seen_platform_bans GUARDED_BY(cs){
        SeenBanInventorySize};

    CMasternodeMetaInfo& GetMetaInfo(const uint256& proTxHash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    const CMasternodeMetaInfo& GetMetaInfoOrDefault(const uint256& proTxHash) const EXCLUSIVE_LOCKS_REQUIRED(cs);

public:
    CMasternodeMetaMan(const CMasternodeMetaMan&) = delete;
    CMasternodeMetaMan& operator=(const CMasternodeMetaMan&) = delete;
    CMasternodeMetaMan();
    ~CMasternodeMetaMan();

    bool LoadCache(bool load_cache);

    bool IsValid() const { return is_valid; }

    CMasternodeMetaInfo GetInfo(const uint256& proTxHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // We keep track of dsq (mixing queues) count to avoid using same masternodes for mixing too often.
    // MN's threshold is calculated as the last dsq count this specific masternode was used in a mixing
    // session plus a margin of 20% of masternode count. In other words we expect at least 20% of unique
    // masternodes before we ever see a masternode that we know already mixed someone's funds earlier.
    bool IsMixingThresholdExceeded(const uint256& protx_hash, int mn_count) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void AllowMixing(const uint256& proTxHash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void DisallowMixing(const uint256& proTxHash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool IsValidForMixingTxes(const uint256& protx_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void AddGovernanceVote(const uint256& proTxHash, const uint256& nGovernanceObjectHash);
    void RemoveGovernanceObject(const uint256& nGovernanceObjectHash) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::vector<uint256> GetAndClearDirtyGovernanceObjectHashes() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void SetLastOutboundAttempt(const uint256& protx_hash, int64_t t) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void SetLastOutboundSuccess(const uint256& protx_hash, int64_t t) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    int64_t GetLastOutboundAttempt(const uint256& protx_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    int64_t GetLastOutboundSuccess(const uint256& protx_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool OutboundFailedTooManyTimes(const uint256& protx_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool IsPlatformBanned(const uint256& protx_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool ResetPlatformBan(const uint256& protx_hash, int height) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool SetPlatformBan(const uint256& inv_hash, PlatformBanMessage&& msg) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool AlreadyHavePlatformBan(const uint256& inv_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    std::optional<PlatformBanMessage> GetPlatformBan(const uint256& inv_hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // CoinJoin masternode tracking
    void AddUsedMasternode(const uint256& proTxHash) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void RemoveUsedMasternodes(size_t count) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    size_t GetUsedMasternodesCount() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool IsUsedMasternode(const uint256& proTxHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

#endif // BITCOIN_MASTERNODE_META_H
