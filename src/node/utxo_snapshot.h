// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_UTXO_SNAPSHOT_H
#define BITCOIN_NODE_UTXO_SNAPSHOT_H

#include <uint256.h>
#include <serialize.h>

//! Metadata describing a serialized version of a UTXO set from which an
//! assumeutxo CChainState can be constructed.
class SnapshotMetadata
{
public:
    //! The hash of the block that reflects the tip of the chain for the
    //! UTXO set contained in this snapshot.
    uint256 m_base_blockhash;

    //! The number of coins in the UTXO set contained in this snapshot. Used
    //! during snapshot load to estimate progress of UTXO set reconstruction.
    uint64_t m_coins_count = 0;

    SnapshotMetadata() { }
    SnapshotMetadata(
        const uint256& base_blockhash,
        uint64_t coins_count,
        unsigned int nchaintx) :
            m_base_blockhash(base_blockhash),
            m_coins_count(coins_count) { }

    SERIALIZE_METHODS(SnapshotMetadata, obj) { READWRITE(obj.m_base_blockhash, obj.m_coins_count); }
};

#endif // BITCOIN_NODE_UTXO_SNAPSHOT_H
