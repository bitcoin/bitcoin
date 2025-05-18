// Copyright (c) 2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_NETGROUP_H
#define TORTOISECOIN_NETGROUP_H

#include <netaddress.h>
#include <uint256.h>

#include <vector>

/**
 * Netgroup manager
 */
class NetGroupManager {
public:
    explicit NetGroupManager(std::vector<bool> asmap)
        : m_asmap{std::move(asmap)}
    {}

    /** Get a checksum identifying the asmap being used. */
    uint256 GetAsmapChecksum() const;

    /**
     * Get the canonical identifier of the network group for address.
     *
     * The groups are assigned in a way where it should be costly for an attacker to
     * obtain addresses with many different group identifiers, even if it is cheap
     * to obtain addresses with the same identifier.
     *
     * @note No two connections will be attempted to addresses with the same network
     *       group.
     */
    std::vector<unsigned char> GetGroup(const CNetAddr& address) const;

    /**
     *  Get the autonomous system on the BGP path to address.
     *
     *  The ip->AS mapping depends on how asmap is constructed.
     */
    uint32_t GetMappedAS(const CNetAddr& address) const;

    /**
     *  Analyze and log current health of ASMap based buckets.
     */
    void ASMapHealthCheck(const std::vector<CNetAddr>& clearnet_addrs) const;

    /**
     *  Indicates whether ASMap is being used for clearnet bucketing.
     */
    bool UsingASMap() const;

private:
    /** Compressed IP->ASN mapping, loaded from a file when a node starts.
     *
     * This mapping is then used for bucketing nodes in Addrman and for
     * ensuring we connect to a diverse set of peers in Connman. The map is
     * empty if no file was provided.
     *
     * If asmap is provided, nodes will be bucketed by AS they belong to, in
     * order to make impossible for a node to connect to several nodes hosted
     * in a single AS. This is done in response to Erebus attack, but also to
     * generally diversify the connections every node creates, especially
     * useful when a large fraction of nodes operate under a couple of cloud
     * providers.
     *
     * If a new asmap is provided, the existing addrman records are
     * re-bucketed.
     *
     * This is initialized in the constructor, const, and therefore is
     * thread-safe. */
    const std::vector<bool> m_asmap;
};

#endif // TORTOISECOIN_NETGROUP_H
