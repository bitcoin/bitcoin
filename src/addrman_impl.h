// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRMAN_IMPL_H
#define BITCOIN_ADDRMAN_IMPL_H

class AddrManImpl
{
public:
    AddrManImpl(std::vector<bool>&& asmap, bool deterministic, int32_t consistency_check_ratio);

    ~AddrManImpl();

    template <typename Stream>
    void Serialize(Stream& s_) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    template <typename Stream>
    void Unserialize(Stream& s_) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    size_t size() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool Add(const std::vector<CAddress> &vAddr, const CNetAddr& source, int64_t nTimePenalty)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void Good(const CService &addr, int64_t nTime)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void Attempt(const CService &addr, bool fCountFailure, int64_t nTime)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void ResolveCollisions() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    CAddrInfo SelectTriedCollision() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    CAddrInfo Select(bool newOnly) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::vector<CAddress> GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void Connected(const CService &addr, int64_t nTime)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void SetServices(const CService &addr, ServiceFlags nServices)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    const std::vector<bool>& GetAsmap() const;

    friend class CAddrManTest;
    friend class CAddrManDeterministic;

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
    };

    //! The maximum format this software knows it can unserialize. Also, we always serialize
    //! in this format.
    //! The format (first byte in the serialized stream) can be higher than this and
    //! still this software may be able to unserialize the file - if the second byte
    //! (see `lowest_compatible` in `Unserialize()`) is less or equal to this.
    static constexpr Format FILE_FORMAT = Format::V3_BIP155;

    //! The initial value of a field that is incremented every time an incompatible format
    //! change is made (such that old software versions would not be able to parse and
    //! understand the new file format). This is 32 because we overtook the "key size"
    //! field which was 32 historically.
    //! @note Don't increment this. Increment `lowest_compatible` in `Serialize()` instead.
    static constexpr uint8_t INCOMPATIBILITY_BASE = 32;

    //! last used nId
    int nIdCount GUARDED_BY(cs){0};

    //! table with information about all nIds
    std::unordered_map<int, CAddrInfo> mapInfo GUARDED_BY(cs);

    //! find an nId based on its network address
    std::unordered_map<CNetAddr, int, CNetAddrHash> mapAddr GUARDED_BY(cs);

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
    int64_t nLastGood GUARDED_BY(cs){1};

    //! Holds addrs inserted into tried table that collide with existing entries. Test-before-evict discipline used to resolve these collisions.
    std::set<int> m_tried_collisions;

    /** Perform consistency checks every m_consistency_check_ratio operations (if non-zero). */
    const int32_t m_consistency_check_ratio;

    // Compressed IP->ASN mapping, loaded from a file when a node starts.
    // Should be always empty if no file was provided.
    // This mapping is then used for bucketing nodes in Addrman.
    //
    // If asmap is provided, nodes will be bucketed by
    // AS they belong to, in order to make impossible for a node
    // to connect to several nodes hosted in a single AS.
    // This is done in response to Erebus attack, but also to generally
    // diversify the connections every node creates,
    // especially useful when a large fraction of nodes
    // operate under a couple of cloud providers.
    //
    // If a new asmap was provided, the existing records
    // would be re-bucketed accordingly.
    const std::vector<bool> m_asmap;

    //! Find an entry.
    CAddrInfo* Find(const CNetAddr& addr, int *pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Create a new entry and add it to the internal data structures mapInfo, mapAddr and vRandom.
    CAddrInfo* Create(const CAddress &addr, const CNetAddr &addrSource, int *pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Swap two elements in vRandom.
    void SwapRandom(unsigned int nRandomPos1, unsigned int nRandomPos2) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Delete an entry. It must not be in tried, and have refcount 0.
    void Delete(int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Clear a position in a "new" table. This is the only place where entries are actually deleted.
    void ClearNew(int nUBucket, int nUBucketPos) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Move an entry from the "new" table(s) to the "tried" table
    void MakeTried(CAddrInfo& info, int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Mark an entry "good", possibly moving it from "new" to "tried".
    void Good_(const CService &addr, bool test_before_evict, int64_t time) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Add an entry to the "new" table.
    bool Add_(const CAddress &addr, const CNetAddr& source, int64_t nTimePenalty) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Mark an entry as attempted to connect.
    void Attempt_(const CService &addr, bool fCountFailure, int64_t nTime) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Select an address to connect to, if newOnly is set to true, only the new table is selected from.
    CAddrInfo Select_(bool newOnly) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /**
     * Return all or many randomly selected addresses, optionally by network.
     *
     * @param[out] vAddr         Vector of randomly selected addresses from vRandom.
     * @param[in] max_addresses  Maximum number of addresses to return (0 = all).
     * @param[in] max_pct        Maximum percentage of addresses to return (0 = all).
     * @param[in] network        Select only addresses of this network (nullopt = all).
     */
    void GetAddr_(std::vector<CAddress>& vAddr, size_t max_addresses, size_t max_pct, std::optional<Network> network) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** We have successfully connected to this peer. Calling this function
     *  updates the CAddress's nTime, which is used in our IsTerrible()
     *  decisions and gossiped to peers. Callers should be careful that updating
     *  this information doesn't leak topology information to network spies.
     *
     *  net_processing calls this function when it *disconnects* from a peer to
     *  not leak information about currently connected peers.
     *
     * @param[in]   addr     The address of the peer we were connected to
     * @param[in]   nTime    The time that we were last connected to this peer
     */
    void Connected_(const CService& addr, int64_t nTime) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Update an entry's service bits.
    void SetServices_(const CService &addr, ServiceFlags nServices) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! See if any to-be-evicted tried table entries have been tested and if so resolve the collisions.
    void ResolveCollisions_() EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Return a random to-be-evicted tried table address.
    CAddrInfo SelectTriedCollision_() EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Consistency check, taking into account m_consistency_check_ratio. Will std::abort if an inconsistency is detected.
    void Check() const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Perform consistency check, regardless of m_consistency_check_ratio.
    //! @returns an error code or zero.
    int ForceCheckAddrman() const EXCLUSIVE_LOCKS_REQUIRED(cs);
};

#endif // BITCOIN_ADDRMAN_IMPL_H
