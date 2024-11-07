// Copyright (c) 2012 Pieter Wuille
// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRMAN_H
#define BITCOIN_ADDRMAN_H

#include <netaddress.h>
#include <netgroup.h>
#include <protocol.h>
#include <streams.h>
#include <util/time.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

class InvalidAddrManVersionError : public std::ios_base::failure
{
public:
    InvalidAddrManVersionError(std::string msg) : std::ios_base::failure(msg) { }
};

class AddrManImpl;
class AddrInfo;

/** Default for -checkaddrman */
static constexpr int32_t DEFAULT_ADDRMAN_CONSISTENCY_CHECKS{0};

/** Location information for an address in AddrMan */
struct AddressPosition {
    // Whether the address is in the new or tried table
    const bool tried;

    // Addresses in the tried table should always have a multiplicity of 1.
    // Addresses in the new table can have multiplicity between 1 and
    // ADDRMAN_NEW_BUCKETS_PER_ADDRESS
    const int multiplicity;

    // If the address is in the new table, the bucket and position are
    // populated based on the first source who sent the address.
    // In certain edge cases, this may not be where the address is currently
    // located.
    const int bucket;
    const int position;

    bool operator==(AddressPosition other) {
        return std::tie(tried, multiplicity, bucket, position) ==
               std::tie(other.tried, other.multiplicity, other.bucket, other.position);
    }
    explicit AddressPosition(bool tried_in, int multiplicity_in, int bucket_in, int position_in)
        : tried{tried_in}, multiplicity{multiplicity_in}, bucket{bucket_in}, position{position_in} {}
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
class AddrMan
{
protected:
    const std::unique_ptr<AddrManImpl> m_impl;

public:
    explicit AddrMan(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio);

    ~AddrMan();

    template <typename Stream>
    void Serialize(Stream& s_) const;

    template <typename Stream>
    void Unserialize(Stream& s_);

    /**
    * Return size information about addrman.
    *
    * @param[in] net              Select addresses only from specified network (nullopt = all)
    * @param[in] in_new           Select addresses only from one table (true = new, false = tried, nullopt = both)
    * @return                     Number of unique addresses that match specified options.
    */
    size_t Size(std::optional<Network> net = std::nullopt, std::optional<bool> in_new = std::nullopt) const;

    /**
     * Attempt to add one or more addresses to addrman's new table.
     * If an address already exists in addrman, the existing entry may be updated
     * (e.g. adding additional service flags). If the existing entry is in the new table,
     * it may be added to more buckets, improving the probability of selection.
     *
     * @param[in] vAddr           Address records to attempt to add.
     * @param[in] source          The address of the node that sent us these addr records.
     * @param[in] time_penalty    A "time penalty" to apply to the address record's nTime. If a peer
     *                            sends us an address record with nTime=n, then we'll add it to our
     *                            addrman with nTime=(n - time_penalty).
     * @return    true if at least one address is successfully added, or added to an additional bucket. Unaffected by updates. */
    bool Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty = 0s);

    /**
     * Mark an address record as accessible and attempt to move it to addrman's tried table.
     *
     * @param[in] addr            Address record to attempt to move to tried table.
     * @param[in] time            The time that we were last connected to this peer.
     * @return    true if the address is successfully moved from the new table to the tried table.
     */
    bool Good(const CService& addr, NodeSeconds time = Now<NodeSeconds>());

    //! Mark an entry as connection attempted to.
    void Attempt(const CService& addr, bool fCountFailure, NodeSeconds time = Now<NodeSeconds>());

    //! See if any to-be-evicted tried table entries have been tested and if so resolve the collisions.
    void ResolveCollisions();

    /**
     * Randomly select an address in the tried table that another address is
     * attempting to evict.
     *
     * @return CAddress The record for the selected tried peer.
     *         seconds  The last time we attempted to connect to that peer.
     */
    std::pair<CAddress, NodeSeconds> SelectTriedCollision();

    /**
     * Choose an address to connect to.
     *
     * @param[in] new_only Whether to only select addresses from the new table. Passing `true` returns
     *                     an address from the new table or an empty pair. Passing `false` will return an
     *                     empty pair or an address from either the new or tried table (it does not
     *                     guarantee a tried entry).
     * @param[in] networks Select only addresses of these networks (empty = all). Passing networks may
     *                     slow down the search.
     * @return    CAddress The record for the selected peer.
     *            seconds  The last time we attempted to connect to that peer.
     */
    std::pair<CAddress, NodeSeconds> Select(bool new_only = false, const std::unordered_set<Network>& networks = {}) const;

    /**
     * Return all or many randomly selected addresses, optionally by network.
     *
     * @param[in] max_addresses  Maximum number of addresses to return (0 = all).
     * @param[in] max_pct        Maximum percentage of addresses to return (0 = all). Value must be from 0 to 100.
     * @param[in] network        Select only addresses of this network (nullopt = all).
     * @param[in] filtered       Select only addresses that are considered good quality (false = all).
     *
     * @return                   A vector of randomly selected addresses from vRandom.
     */
    std::vector<CAddress> GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network, const bool filtered = true) const;

    /**
     * Returns an information-location pair for all addresses in the selected addrman table.
     * If an address appears multiple times in the new table, an information-location pair
     * is returned for each occurrence. Addresses only ever appear once in the tried table.
     *
     * @param[in] from_tried     Selects which table to return entries from.
     *
     * @return                   A vector consisting of pairs of AddrInfo and AddressPosition.
     */
    std::vector<std::pair<AddrInfo, AddressPosition>> GetEntries(bool from_tried) const;

    /** We have successfully connected to this peer. Calling this function
     *  updates the CAddress's nTime, which is used in our IsTerrible()
     *  decisions and gossiped to peers. Callers should be careful that updating
     *  this information doesn't leak topology information to network spies.
     *
     *  net_processing calls this function when it *disconnects* from a peer to
     *  not leak information about currently connected peers.
     *
     * @param[in]   addr     The address of the peer we were connected to
     * @param[in]   time     The time that we were last connected to this peer
     */
    void Connected(const CService& addr, NodeSeconds time = Now<NodeSeconds>());

    //! Update an entry's service bits.
    void SetServices(const CService& addr, ServiceFlags nServices);

    /** Test-only function
     * Find the address record in AddrMan and return information about its
     * position.
     * @param[in] addr       The address record to look up.
     * @return               Information about the address record in AddrMan
     *                       or nullopt if address is not found.
     */
    std::optional<AddressPosition> FindAddressEntry(const CAddress& addr);
};

#endif // BITCOIN_ADDRMAN_H
