// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETGROUP_H
#define BITCOIN_NETGROUP_H

#include <netaddress.h>

#include <vector>

/**
 * Netgroup manager
 */
class NetGroupManager {
public:
    explicit NetGroupManager(std::vector<bool> asmap)
        : m_asmap{std::move(asmap)}
    {}

    /* Get a reference to (const) asmap. May be held as long as NetGroupManager
     * exists, since the data is const. */
    const std::vector<bool>& GetAsmap() const { return m_asmap; }

    std::vector<unsigned char> GetGroup(const CNetAddr& address) const;

    uint32_t GetMappedAS(const CNetAddr& address) const;

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

#endif // BITCOIN_NETGROUP_H
