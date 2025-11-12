// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_SWIFTSYNC_H
#define BITCOIN_SWIFTSYNC_H
#include <arith_uint256.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <uint256.h>
#include <util/hasher.h>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace swiftsync {
inline constexpr std::array<uint8_t, 4> FILE_MAGIC = {0x55, 0x54, 0x58, 0x4f};
const uint8_t FILE_VERSION = 0x00;
// file magic length + version + height
const uint64_t FILE_HEADER_LEN = 9;
/** An aggregate for the SwiftSync protocol.
 * This class is intentionally left opaque, as internal changes may occur,
 * but all aggregates will have the concept of "create" and "spending" an
 * outpoint.
 *
 * The current implementation uses the existing `SaltedOutpointHasher` and
 * maintains two rotating integers.
 * */
class Aggregate
{
    arith_uint256 m_created{};
    arith_uint256 m_spent{};
    SaltedOutpointHasher m_hasher{};

public:
    bool IsBalanced() const { return m_created == m_spent; }
    void Create(const COutPoint& outpoint) { m_created += m_hasher(outpoint); }
    void Spend(const COutPoint& outpoint) { m_spent += m_hasher(outpoint); }
};

/**
 * Write the hints of a single block.
 */
class BlockHintsWriter
{
private:
    std::vector<uint8_t> m_hints{};
    uint8_t m_curr_byte{};
    uint32_t m_bits_written{};
    void WriteBit(uint8_t bit) noexcept;

public:
    /** Push a `1` to the stack. */
    void PushHighBit() noexcept { return WriteBit(0x01); }
    /** Push a `0` to the stack. */
    void PushLowBit() noexcept { return WriteBit(0x00); }
    /** Push the hints to a file and commit the changes. */
    bool EncodeToFile(AutoFile& file);
};

/**
 * Create a new hint file for writing.
 */
class HintsfileWriter
{
private:
    AutoFile m_file;
    uint32_t m_index{};

public:
    // Create a new hint file writer that will encode `preallocate` number of blocks.
    HintsfileWriter(AutoFile& file, uint32_t preallocate);
    ~HintsfileWriter()
    {
        (void)m_file.fclose();
    }
    // Write the next hints to file.
    bool WriteNextUnspents(BlockHintsWriter& hints, uint32_t height);
    // Close the underlying file.
    int Close() { return m_file.fclose(); };
};

class BlockHints
{
private:
    std::vector<bool> m_hints{};
    uint32_t m_index{};

public:
    BlockHints(std::vector<bool> hints) : m_hints{hints} {};
    void Next() noexcept { ++m_index; };
    bool IsCurrUnspent() const { return m_hints.at(m_index); }
};

/**
 * A file that encodes the UTXO set state at a particular block height.
 */
class HintsfileReader
{
private:
    AutoFile m_file;
    uint32_t m_stop_height;
    std::unordered_map<uint32_t, size_t> m_height_to_file_pos;

public:
    HintsfileReader(AutoFile& file);
    HintsfileReader(HintsfileReader&& o) noexcept;
    ~HintsfileReader()
    {
        (void)m_file.fclose();
    }
    /** Read the hints for the specified block height. */
    BlockHints ReadBlock(uint32_t height);
    /** The height this file encodes up to. */
    uint32_t StopHeight() const noexcept { return m_stop_height; };
};
} // namespace swiftsync
#endif // BITCOIN_SWIFTSYNC_H
