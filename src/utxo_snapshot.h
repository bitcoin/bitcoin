// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTXO_SNAPSHOT_H
#define BITCOIN_UTXO_SNAPSHOT_H

#include <kernel/messagestartchars.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/chaintype.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ios>
#include <string>

// UTXO set snapshot magic bytes
inline constexpr std::array<uint8_t, 5> SNAPSHOT_MAGIC_BYTES = {'u', 't', 'x', 'o', 0xff};

//! Metadata describing a serialized version of a UTXO set from which an
//! assumeutxo Chainstate can be constructed.
//! All metadata fields come from an untrusted file, so must be validated
//! before being used. Thus, new fields should be added only if needed.
class SnapshotMetadata
{
    static constexpr uint16_t VERSION{2};
    static inline bool IsSupportedVersion(uint16_t version) {
        return version == VERSION;
    }

public:
    //! The network magic from the snapshot file. See CheckNetwork()
    MessageStartChars m_network_magic{};

    //! The hash of the block that reflects the tip of the chain for the
    //! UTXO set contained in this snapshot.
    uint256 m_base_blockhash;

    //! The number of coins in the UTXO set contained in this snapshot. Used
    //! during snapshot load to estimate progress of UTXO set reconstruction.
    uint64_t m_coins_count = 0;

    SnapshotMetadata() = default;
    SnapshotMetadata(
        const MessageStartChars network_magic,
        const uint256& base_blockhash,
        uint64_t coins_count) :
            m_network_magic(network_magic),
            m_base_blockhash(base_blockhash),
            m_coins_count(coins_count) { }

    /** Throws std::ios_base::failure if the file doesn't match
     * `expected`. Uses the same exception as a bad read, so callers
     * can catch both the same way. */
    void CheckNetwork(const MessageStartChars& expected) const
    {
        if (std::equal(m_network_magic.begin(), m_network_magic.end(), expected.begin())) return;
        const auto metadata_network{GetNetworkForMagic(m_network_magic)};
        if (metadata_network) {
            const std::string network_string{ChainTypeToString(metadata_network.value())};
            const auto node_network{GetNetworkForMagic(expected)};
            const std::string node_network_string{ChainTypeToString(node_network.value())};
            throw std::ios_base::failure(strprintf("The network of the snapshot (%s) does not match the network of this node (%s).", network_string, node_network_string));
        }
        throw std::ios_base::failure("This snapshot has been created for an unrecognized network. This could be a custom signet, a new testnet or possibly caused by data corruption.");
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        s << SNAPSHOT_MAGIC_BYTES;
        s << VERSION;
        s << m_network_magic;
        s << m_base_blockhash;
        s << m_coins_count;
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Read the snapshot magic bytes
        std::array<uint8_t, SNAPSHOT_MAGIC_BYTES.size()> snapshot_magic;
        s >> snapshot_magic;
        if (snapshot_magic != SNAPSHOT_MAGIC_BYTES) {
            throw std::ios_base::failure("Invalid UTXO set snapshot magic bytes. Please check if this is indeed a snapshot file or if you are using an outdated snapshot format.");
        }

        // Read the version
        uint16_t version;
        s >> version;
        if (!IsSupportedVersion(version)) {
            throw std::ios_base::failure(strprintf("Version of snapshot %s does not match any of the supported versions.", version));
        }

        // Read the network magic (pchMessageStart)
        s >> m_network_magic;

        s >> m_base_blockhash;
        s >> m_coins_count;
    }
};

#endif // BITCOIN_UTXO_SNAPSHOT_H
