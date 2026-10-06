// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_BLOCK_SEQ_H
#define BITCOIN_INDEX_BLOCK_SEQ_H

#include <consensus/consensus.h>
#include <serialize.h>
#include <uint256.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <string>

namespace block_seq {

inline const std::string DB_NEXT_BLOCK_SEQ{"next_block_seq"};

//! Empty value of a hashed row, whose position is encoded in its key.
inline constexpr std::array<std::byte, 0> EMPTY_VALUE{};

//! Serialized size of a block header, the offset of the first byte after it.
inline constexpr uint32_t BLOCK_HEADER_SIZE{80};

//! The location of a transaction: the sequence number of the block that contains it
//! and the transaction's serialized byte offset from the start of that block
//! (including the header), so the on-disk position is simply
//! block_data_pos + tx_offset_in_block.
struct BlockTxPosition {
    uint32_t block_seq{0};
    uint32_t tx_offset_in_block{0};

    friend bool operator==(const BlockTxPosition&, const BlockTxPosition&) = default;

    // tx_offset is encoded in 3-byte big-endian integer.
    // This can hold up to 16,777,215, which is >4x the maximum serialized block size
    static constexpr uint32_t TX_OFFSET_SIZE{3};
    static_assert(MAX_BLOCK_SERIALIZED_SIZE <= BigEndianFormatter<TX_OFFSET_SIZE>::MAX);

    SERIALIZE_METHODS(BlockTxPosition, obj)
    {
        READWRITE(VARINT(obj.block_seq),
                  Using<BigEndianFormatter<TX_OFFSET_SIZE>>(obj.tx_offset_in_block));
    }
};

//! Key for looking up the hash of the block with the given sequence number.
template <uint8_t PREFIX>
struct BlockSeqKey {
    uint32_t block_seq{0};

    SERIALIZE_METHODS(BlockSeqKey, obj)
    {
        uint8_t prefix{PREFIX};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != PREFIX) throw std::ios_base::failure("Invalid format for block seq key");
        READWRITE(VARINT(obj.block_seq));
    }
};

//! Key for looking up the sequence number assigned to the block with the given hash.
template <uint8_t PREFIX>
struct BlockHashKey {
    uint256 block_hash;

    SERIALIZE_METHODS(BlockHashKey, obj)
    {
        uint8_t prefix{PREFIX};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != PREFIX) throw std::ios_base::failure("Invalid format for block hash key");
        READWRITE(obj.block_hash);
    }
};

inline constexpr int HASH_PREFIX_SIZE{5};
using HashKeyPrefix = uint64_t;

constexpr HashKeyPrefix TruncateHash(uint64_t hash)
{
    return hash >> (8 * (sizeof(HashKeyPrefix) - HASH_PREFIX_SIZE));
}

template <uint8_t PREFIX>
struct HashedPositionKey {
    HashKeyPrefix hash_prefix{0};
    BlockTxPosition pos;

    SERIALIZE_METHODS(HashedPositionKey, obj)
    {
        uint8_t prefix{PREFIX};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != PREFIX) throw std::ios_base::failure("Invalid format for hashed position key");
        READWRITE(Using<BigEndianFormatter<HASH_PREFIX_SIZE>>(obj.hash_prefix), obj.pos);
    }
};

} // namespace block_seq

#endif // BITCOIN_INDEX_BLOCK_SEQ_H
