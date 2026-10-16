// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_UTXO_SNAPSHOT_H
#define BITCOIN_NODE_UTXO_SNAPSHOT_H

#include <kernel/messagestartchars.h>
#include <uint256.h>

#include <array>
#include <cstdint>

// UTXO set snapshot magic bytes
inline constexpr std::array<uint8_t, 5> SNAPSHOT_MAGIC_BYTES = {'u', 't', 'x', 'o', 0xff};

namespace node {
//! Metadata describing a serialized UTXO set
class SnapshotMetadata
{
    static constexpr uint16_t VERSION{2};
    const MessageStartChars m_network_magic;
public:
    //! The hash of the block that reflects the tip of the chain for the
    //! UTXO set contained in this snapshot.
    uint256 m_base_blockhash;


    //! The number of coins in the UTXO set contained in this snapshot.
    uint64_t m_coins_count = 0;

    SnapshotMetadata(
        const MessageStartChars network_magic,
        const uint256& base_blockhash,
        uint64_t coins_count) :
            m_network_magic(network_magic),
            m_base_blockhash(base_blockhash),
            m_coins_count(coins_count) { }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        s << SNAPSHOT_MAGIC_BYTES;
        s << VERSION;
        s << m_network_magic;
        s << m_base_blockhash;
        s << m_coins_count;
    }

};

} // namespace node

#endif // BITCOIN_NODE_UTXO_SNAPSHOT_H
