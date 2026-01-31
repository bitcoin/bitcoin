// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETGROUP_H
#define BITCOIN_NETGROUP_H

#include <netaddress.h>
#include <uint256.h>

#include <cstddef>
#include <vector>

/**
 * Netgroup manager
 */
class NetGroupManager {
public:
    NetGroupManager(const NetGroupManager&) = delete;
    NetGroupManager(NetGroupManager&&) = default;
    NetGroupManager& operator=(const NetGroupManager&) = delete;
    NetGroupManager& operator=(NetGroupManager&&) = delete;

    static NetGroupManager WithEmbeddedAsmap(std::span<const std::byte> asmap) {
        return NetGroupManager(asmap, {});
    }

    static NetGroupManager WithLoadedAsmap(std::vector<std::byte>&& asmap) {
        return NetGroupManager(std::span{asmap}, std::move(asmap));
    }

    static NetGroupManager NoAsmap() {
        return NetGroupManager({}, {});
    }

    /** Get the asmap version, a checksum identifying the asmap being used. */
    uint256 GetAsmapVersion() const;

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
    /** Compressed IP->ASN mapping.
     *
     * Data may be loaded from a file when a node starts or embedded in the
     * binary.
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
     * thread-safe. m_asmap can either point to m_loaded_asmap which holds
     * data loaded from an external file at runtime or it can point to embedded
     * asmap data.
     */
    const std::span<const std::byte> m_asmap;
    std::vector<std::byte> m_loaded_asmap;

    explicit NetGroupManager(std::span<const std::byte> embedded_asmap, std::vector<std::byte>&& loaded_asmap)
        : m_asmap{embedded_asmap},
          m_loaded_asmap{std::move(loaded_asmap)}
    {
        assert(m_loaded_asmap.empty() || m_asmap.data() == m_loaded_asmap.data());
    }
};

#endif // BITCOIN_NETGROUP_H
