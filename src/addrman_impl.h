// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRMAN_IMPL_H
#define BITCOIN_ADDRMAN_IMPL_H

#include <logging.h>
#include <logging/timer.h>
#include <netaddress.h>
#include <protocol.h>
#include <serialize.h>
#include <sync.h>
#include <uint256.h>
#include <util/time.h>

#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/** Total number of buckets for tried addresses */
static constexpr int32_t ADDRMAN_TRIED_BUCKET_COUNT_LOG2{8};
static constexpr int ADDRMAN_TRIED_BUCKET_COUNT{1 << ADDRMAN_TRIED_BUCKET_COUNT_LOG2};
/** Total number of buckets for new addresses */
static constexpr int32_t ADDRMAN_NEW_BUCKET_COUNT_LOG2{10};
static constexpr int ADDRMAN_NEW_BUCKET_COUNT{1 << ADDRMAN_NEW_BUCKET_COUNT_LOG2};
/** Maximum allowed number of entries in buckets for new and tried addresses */
static constexpr int32_t ADDRMAN_BUCKET_SIZE_LOG2{6};
static constexpr int ADDRMAN_BUCKET_SIZE{1 << ADDRMAN_BUCKET_SIZE_LOG2};

/**
 * Extended statistics about a CAddress
 */
class AddrInfo : public CAddress
{
public:
    //! last try whatsoever by us (memory only)
    NodeSeconds m_last_try{0s};

    //! last counted attempt (memory only)
    NodeSeconds m_last_count_attempt{0s};

    //! where knowledge about this address first came from
    CNetAddr source;

    //! last successful connection by us
    NodeSeconds m_last_success{0s};

    //! connection attempts since last successful attempt
    int nAttempts{0};

    //! reference count in new sets (memory only)
    int nRefCount{0};

    //! in tried set? (memory only)
    bool fInTried{false};

    //! position in vRandom
    mutable int nRandomPos{-1};

    SERIALIZE_METHODS(AddrInfo, obj)
    {
        READWRITE(AsBase<CAddress>(obj), obj.source, Using<ChronoFormatter<int64_t>>(obj.m_last_success), obj.nAttempts);
    }

    AddrInfo(const CAddress &addrIn, const CNetAddr &addrSource) : CAddress(addrIn), source(addrSource)
    {
    }

    AddrInfo() : CAddress(), source()
    {
    }

    //! Calculate in which "tried" bucket this entry belongs
    int GetTriedBucket(const uint256& nKey, const NetGroupManager& netgroupman) const;

    //! Calculate in which "new" bucket this entry belongs, given a certain source
    int GetNewBucket(const uint256& nKey, const CNetAddr& src, const NetGroupManager& netgroupman) const;

    //! Calculate in which "new" bucket this entry belongs, using its default source
    int GetNewBucket(const uint256& nKey, const NetGroupManager& netgroupman) const
    {
        return GetNewBucket(nKey, source, netgroupman);
    }

    //! Calculate in which position of a bucket to store this entry.
    int GetBucketPosition(const uint256 &nKey, bool fNew, int bucket) const;

    //! Determine whether the statistics about this entry are bad enough so that it can just be deleted
    bool IsTerrible(NodeSeconds now = Now<NodeSeconds>()) const;

    //! Calculate the relative chance this entry should be given when selecting nodes to connect to
    double GetChance(NodeSeconds now = Now<NodeSeconds>()) const;
};

class AddrManImpl
{
public:
    AddrManImpl(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio);

    ~AddrManImpl();

    template <typename Stream>
    void Serialize(Stream& s_) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    template <typename Stream>
    void Unserialize(Stream& s_) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    size_t Size(std::optional<Network> net, std::optional<bool> in_new) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool Good(const CService& addr, NodeSeconds time)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void Attempt(const CService& addr, bool fCountFailure, NodeSeconds time)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void ResolveCollisions() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::pair<CAddress, NodeSeconds> SelectTriedCollision() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::pair<CAddress, NodeSeconds> Select(bool new_only, const std::unordered_set<Network>& networks) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::vector<CAddress> GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network, const bool filtered = true) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::vector<std::pair<AddrInfo, AddressPosition>> GetEntries(bool from_tried) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void Connected(const CService& addr, NodeSeconds time)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void SetServices(const CService& addr, ServiceFlags nServices)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::optional<AddressPosition> FindAddressEntry(const CAddress& addr)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    friend class AddrManDeterministic;

private:
    //! A mutex to protect the inner data structures.
    mutable Mutex cs;

    //! Source of random numbers for randomization in inner loops
    mutable FastRandomContext insecure_rand GUARDED_BY(cs);

    //! secret key to randomize bucket select with
    uint256 nKey;

    //! Serialization versions.
    enum Format : uint8_t {
        V0_HISTORICAL = 0,    //!< historic format, before commit e6b343d88
        V1_DETERMINISTIC = 1, //!< for pre-asmap files
        V2_ASMAP = 2,         //!< for files including asmap version
        V3_BIP155 = 3,        //!< same as V2_ASMAP plus addresses are in BIP155 format
        V4_MULTIPORT = 4,     //!< adds support for multiple ports per IP
    };

    //! The maximum format this software knows it can unserialize. Also, we always serialize
    //! in this format.
    //! The format (first byte in the serialized stream) can be higher than this and
    //! still this software may be able to unserialize the file - if the second byte
    //! (see `lowest_compatible` in `Unserialize()`) is less or equal to this.
    static constexpr Format FILE_FORMAT = Format::V4_MULTIPORT;

    //! The initial value of a field that is incremented every time an incompatible format
    //! change is made (such that old software versions would not be able to parse and
    //! understand the new file format). This is 32 because we overtook the "key size"
    //! field which was 32 historically.
    //! @note Don't increment this. Increment `lowest_compatible` in `Serialize()` instead.
    static constexpr uint8_t INCOMPATIBILITY_BASE = 32;

    //! last used nId
    int nIdCount GUARDED_BY(cs){0};

    //! table with information about all nIds
    std::unordered_map<int, AddrInfo> mapInfo GUARDED_BY(cs);

    //! find an nId based on its network address and port.
    std::unordered_map<CService, int, CServiceHash> mapAddr GUARDED_BY(cs);

    //! randomly-ordered vector of all nIds
    //! This is mutable because it is unobservable outside the class, so any
    //! changes to it (even in const methods) are also unobservable.
    mutable std::vector<int> vRandom GUARDED_BY(cs);

    // number of "tried" entries
    int nTried GUARDED_BY(cs){0};

    //! list of "tried" buckets
    int vvTried[ADDRMAN_TRIED_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE] GUARDED_BY(cs);

    //! number of (unique) "new" entries
    int nNew GUARDED_BY(cs){0};

    //! list of "new" buckets
    int vvNew[ADDRMAN_NEW_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE] GUARDED_BY(cs);

    //! last time Good was called (memory only). Initially set to 1 so that "never" is strictly worse.
    NodeSeconds m_last_good GUARDED_BY(cs){1s};

    //! Holds addrs inserted into tried table that collide with existing entries. Test-before-evict discipline used to resolve these collisions.
    std::set<int> m_tried_collisions;

    /** Perform consistency checks every m_consistency_check_ratio operations (if non-zero). */
    const int32_t m_consistency_check_ratio;

    /** Reference to the netgroup manager. netgroupman must be constructed before addrman and destructed after. */
    const NetGroupManager& m_netgroupman;

    struct NewTriedCount {
        size_t n_new;
        size_t n_tried;
    };

    /** Number of entries in addrman per network and new/tried table. */
    std::unordered_map<Network, NewTriedCount> m_network_counts GUARDED_BY(cs);

    //! Find an entry.
    AddrInfo* Find(const CService& addr, int* pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Create a new entry and add it to the internal data structures mapInfo, mapAddr and vRandom.
    AddrInfo* Create(const CAddress& addr, const CNetAddr& addrSource, int* pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Swap two elements in vRandom.
    void SwapRandom(unsigned int nRandomPos1, unsigned int nRandomPos2) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Delete an entry. It must not be in tried, and have refcount 0.
    void Delete(int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Clear a position in a "new" table. This is the only place where entries are actually deleted.
    void ClearNew(int nUBucket, int nUBucketPos) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Move an entry from the "new" table(s) to the "tried" table
    void MakeTried(AddrInfo& info, int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Attempt to add a single address to addrman's new table.
     *  @see AddrMan::Add() for parameters. */
    bool AddSingle(const CAddress& addr, const CNetAddr& source, std::chrono::seconds time_penalty) EXCLUSIVE_LOCKS_REQUIRED(cs);

    bool Good_(const CService& addr, bool test_before_evict, NodeSeconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);

    bool Add_(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void Attempt_(const CService& addr, bool fCountFailure, NodeSeconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);

    std::pair<CAddress, NodeSeconds> Select_(bool new_only, const std::unordered_set<Network>& networks) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Helper to generalize looking up an addrman entry from either table.
     *
     *  @return  int The nid of the entry. If the addrman position is empty or not found, returns -1.
     * */
    int GetEntry(bool use_tried, size_t bucket, size_t position) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    std::vector<CAddress> GetAddr_(size_t max_addresses, size_t max_pct, std::optional<Network> network, const bool filtered = true) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    std::vector<std::pair<AddrInfo, AddressPosition>> GetEntries_(bool from_tried) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    void Connected_(const CService& addr, NodeSeconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void SetServices_(const CService& addr, ServiceFlags nServices) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void ResolveCollisions_() EXCLUSIVE_LOCKS_REQUIRED(cs);

    std::pair<CAddress, NodeSeconds> SelectTriedCollision_() EXCLUSIVE_LOCKS_REQUIRED(cs);

    std::optional<AddressPosition> FindAddressEntry_(const CAddress& addr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    size_t Size_(std::optional<Network> net, std::optional<bool> in_new) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Consistency check, taking into account m_consistency_check_ratio.
    //! Will std::abort if an inconsistency is detected.
    void Check() const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Perform consistency check, regardless of m_consistency_check_ratio.
    //! @returns an error code or zero.
    int CheckAddrman() const EXCLUSIVE_LOCKS_REQUIRED(cs);
};

#endif // BITCOIN_ADDRMAN_IMPL_H
