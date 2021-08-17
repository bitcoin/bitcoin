// Copyright (c) 2012 Pieter Wuille
// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRMAN_H
#define BITCOIN_ADDRMAN_H

#include <clientversion.h>
#include <config/bitcoin-config.h>
#include <fs.h>
#include <hash.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <streams.h>
#include <sync.h>
#include <timedata.h>
#include <tinyformat.h>
#include <util/system.h>

#include <iostream>
#include <optional>
#include <set>
#include <stdint.h>
#include <unordered_map>
#include <vector>

/** Default for -checkaddrman */
static constexpr int32_t DEFAULT_ADDRMAN_CONSISTENCY_CHECKS{0};

/**
 * Extended statistics about a CAddress
 */
class CAddrInfo : public CAddress
{
public:
    //! last try whatsoever by us (memory only)
    int64_t nLastTry{0};

    //! last counted attempt (memory only)
    int64_t nLastCountAttempt{0};

private:
    //! where knowledge about this address first came from
    CNetAddr source;

    //! last successful connection by us
    int64_t nLastSuccess{0};

    //! connection attempts since last successful attempt
    int nAttempts{0};

    //! reference count in new sets (memory only)
    int nRefCount{0};

    //! in tried set? (memory only)
    bool fInTried{false};

    //! position in vRandom
    mutable int nRandomPos{-1};

    friend class CAddrMan;
    friend class CAddrManDeterministic;

public:

    SERIALIZE_METHODS(CAddrInfo, obj)
    {
        READWRITEAS(CAddress, obj);
        READWRITE(obj.source, obj.nLastSuccess, obj.nAttempts);
    }

    CAddrInfo(const CAddress &addrIn, const CNetAddr &addrSource) : CAddress(addrIn), source(addrSource)
    {
    }

    CAddrInfo() : CAddress(), source()
    {
    }

    //! Calculate in which "tried" bucket this entry belongs
    int GetTriedBucket(const uint256 &nKey, const std::vector<bool> &asmap) const;

    //! Calculate in which "new" bucket this entry belongs, given a certain source
    int GetNewBucket(const uint256 &nKey, const CNetAddr& src, const std::vector<bool> &asmap) const;

    //! Calculate in which "new" bucket this entry belongs, using its default source
    int GetNewBucket(const uint256 &nKey, const std::vector<bool> &asmap) const
    {
        return GetNewBucket(nKey, source, asmap);
    }

    //! Calculate in which position of a bucket to store this entry.
    int GetBucketPosition(const uint256 &nKey, bool fNew, int nBucket) const;

    //! Determine whether the statistics about this entry are bad enough so that it can just be deleted
    bool IsTerrible(int64_t nNow = GetAdjustedTime()) const;

    //! Calculate the relative chance this entry should be given when selecting nodes to connect to
    double GetChance(int64_t nNow = GetAdjustedTime()) const;
};

/** Stochastic address manager
 *
 * Design goals:
 *  * Keep the address tables in-memory, and asynchronously dump the entire table to peers.dat.
 *  * Make sure no (localized) attacker can fill the entire table with his nodes/addresses.
 *
 * To that end:
 *  * Addresses are organized into buckets that can each store up to 64 entries.
 *    * Addresses to which our node has not successfully connected go into 1024 "new" buckets.
 *      * Based on the address range (/16 for IPv4) of the source of information, or if an asmap is provided,
 *        the AS it belongs to (for IPv4/IPv6), 64 buckets are selected at random.
 *      * The actual bucket is chosen from one of these, based on the range in which the address itself is located.
 *      * The position in the bucket is chosen based on the full address.
 *      * One single address can occur in up to 8 different buckets to increase selection chances for addresses that
 *        are seen frequently. The chance for increasing this multiplicity decreases exponentially.
 *      * When adding a new address to an occupied position of a bucket, it will not replace the existing entry
 *        unless that address is also stored in another bucket or it doesn't meet one of several quality criteria
 *        (see IsTerrible for exact criteria).
 *    * Addresses of nodes that are known to be accessible go into 256 "tried" buckets.
 *      * Each address range selects at random 8 of these buckets.
 *      * The actual bucket is chosen from one of these, based on the full address.
 *      * When adding a new good address to an occupied position of a bucket, a FEELER connection to the
 *        old address is attempted. The old entry is only replaced and moved back to the "new" buckets if this
 *        attempt was unsuccessful.
 *    * Bucket selection is based on cryptographic hashing, using a randomly-generated 256-bit key, which should not
 *      be observable by adversaries.
 *    * Several indexes are kept for high performance. Setting m_consistency_check_ratio with the -checkaddrman
 *      configuration option will introduce (expensive) consistency checks for the entire data structure.
 */

//! total number of buckets for tried addresses
static constexpr int32_t ADDRMAN_TRIED_BUCKET_COUNT_LOG2{8};
static constexpr int ADDRMAN_TRIED_BUCKET_COUNT{1 << ADDRMAN_TRIED_BUCKET_COUNT_LOG2};

//! total number of buckets for new addresses
static constexpr int32_t ADDRMAN_NEW_BUCKET_COUNT_LOG2{10};
static constexpr int ADDRMAN_NEW_BUCKET_COUNT{1 << ADDRMAN_NEW_BUCKET_COUNT_LOG2};

//! maximum allowed number of entries in buckets for new and tried addresses
static constexpr int32_t ADDRMAN_BUCKET_SIZE_LOG2{6};
static constexpr int ADDRMAN_BUCKET_SIZE{1 << ADDRMAN_BUCKET_SIZE_LOG2};

/**
 * Stochastical (IP) address manager
 */
class CAddrMan
{
public:
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
    std::vector<bool> m_asmap;

    // Read asmap from provided binary file
    static std::vector<bool> DecodeAsmap(fs::path path);

    template <typename Stream>
    void Serialize(Stream& s_) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    template <typename Stream>
    void Unserialize(Stream& s_) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    explicit CAddrMan(bool deterministic, int32_t consistency_check_ratio);

    ~CAddrMan()
    {
        nKey.SetNull();
    }

    //! Return the number of (unique) addresses in all tables.
    size_t size() const
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs); // TODO: Cache this in an atomic to avoid this overhead
        return vRandom.size();
    }

    //! Add addresses to addrman's new table.
    bool Add(const std::vector<CAddress> &vAddr, const CNetAddr& source, int64_t nTimePenalty = 0)
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        int nAdd = 0;
        Check();
        for (std::vector<CAddress>::const_iterator it = vAddr.begin(); it != vAddr.end(); it++)
            nAdd += Add_(*it, source, nTimePenalty) ? 1 : 0;
        Check();
        if (nAdd) {
            LogPrint(BCLog::ADDRMAN, "Added %i addresses from %s: %i tried, %i new\n", nAdd, source.ToString(), nTried, nNew);
        }
        return nAdd > 0;
    }

    //! Mark an entry as accessible.
    void Good(const CService &addr, int64_t nTime = GetAdjustedTime())
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        Good_(addr, /* test_before_evict */ true, nTime);
        Check();
    }

    //! Mark an entry as connection attempted to.
    void Attempt(const CService &addr, bool fCountFailure, int64_t nTime = GetAdjustedTime())
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        Attempt_(addr, fCountFailure, nTime);
        Check();
    }

    //! See if any to-be-evicted tried table entries have been tested and if so resolve the collisions.
    void ResolveCollisions()
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        ResolveCollisions_();
        Check();
    }

    //! Randomly select an address in tried that another address is attempting to evict.
    CAddrInfo SelectTriedCollision()
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        const CAddrInfo ret = SelectTriedCollision_();
        Check();
        return ret;
    }

    /**
     * Choose an address to connect to.
     */
    CAddrInfo Select(bool newOnly = false) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        const CAddrInfo addrRet = Select_(newOnly);
        Check();
        return addrRet;
    }

    /**
     * Return all or many randomly selected addresses, optionally by network.
     *
     * @param[in] max_addresses  Maximum number of addresses to return (0 = all).
     * @param[in] max_pct        Maximum percentage of addresses to return (0 = all).
     * @param[in] network        Select only addresses of this network (nullopt = all).
     */
    std::vector<CAddress> GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        std::vector<CAddress> vAddr;
        GetAddr_(vAddr, max_addresses, max_pct, network);
        Check();
        return vAddr;
    }

    //! Outer function for Connected_()
    void Connected(const CService &addr, int64_t nTime = GetAdjustedTime())
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        Connected_(addr, nTime);
        Check();
    }

    void SetServices(const CService &addr, ServiceFlags nServices)
        EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        Check();
        SetServices_(addr, nServices);
        Check();
    }

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

    //! Find an entry.
    CAddrInfo* Find(const CNetAddr& addr, int *pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Create a new entry and add it to the internal data structures mapInfo, mapAddr and vRandom.
    CAddrInfo* Create(const CAddress &addr, const CNetAddr &addrSource, int *pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Swap two elements in vRandom.
    void SwapRandom(unsigned int nRandomPos1, unsigned int nRandomPos2) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Move an entry from the "new" table(s) to the "tried" table
    void MakeTried(CAddrInfo& info, int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Delete an entry. It must not be in tried, and have refcount 0.
    void Delete(int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Clear a position in a "new" table. This is the only place where entries are actually deleted.
    void ClearNew(int nUBucket, int nUBucketPos) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Mark an entry "good", possibly moving it from "new" to "tried".
    void Good_(const CService &addr, bool test_before_evict, int64_t time) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Add an entry to the "new" table.
    bool Add_(const CAddress &addr, const CNetAddr& source, int64_t nTimePenalty) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Mark an entry as attempted to connect.
    void Attempt_(const CService &addr, bool fCountFailure, int64_t nTime) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Select an address to connect to, if newOnly is set to true, only the new table is selected from.
    CAddrInfo Select_(bool newOnly) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! See if any to-be-evicted tried table entries have been tested and if so resolve the collisions.
    void ResolveCollisions_() EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Return a random to-be-evicted tried table address.
    CAddrInfo SelectTriedCollision_() EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Consistency check
    void Check() const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);

        const int err = Check_();
        if (err) {
            LogPrintf("ADDRMAN CONSISTENCY CHECK FAILED!!! err=%i\n", err);
            assert(false);
        }
    }

    //! Perform consistency check. Returns an error code or zero.
    int Check_() const EXCLUSIVE_LOCKS_REQUIRED(cs);

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

    friend class CAddrManTest;
    friend class CAddrManDeterministic;
};

#endif // BITCOIN_ADDRMAN_H
