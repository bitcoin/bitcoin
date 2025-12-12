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
#include <util/fs.h>
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
 * Create a new hint file for writing.
 */
class HintsfileWriter
{
private:
    AutoFile m_file;
    uint32_t m_index{};

public:
    // Create a new hint file writer that will encode `preallocate` number of blocks.
    HintsfileWriter(AutoFile& file, const uint32_t& preallocate);
    ~HintsfileWriter()
    {
        (void)m_file.fclose();
    }
    // Write the next hints to file.
    bool WriteNextUnspents(const std::vector<uint64_t>& unspent_offsets, const uint32_t& height);
    // Close the underlying file.
    int Close() { return m_file.fclose(); };
};

/**
 * A file that encodes the UTXO set state at a particular block height.
 */
class HintsfileReader
{
private:
    AutoFile m_file;
    uint32_t m_stop_height;
    std::unordered_map<uint32_t, uint64_t> m_height_to_file_pos;

public:
    HintsfileReader(AutoFile& file);
    HintsfileReader(HintsfileReader&& o) noexcept;
    ~HintsfileReader()
    {
        (void)m_file.fclose();
    }
    /** Read the hints for the specified block height. */
    std::vector<uint64_t> ReadBlock(const uint32_t& height);
    /** The height this file encodes up to. */
    uint32_t StopHeight() { return m_stop_height; };
};

/**
 * Stateful data structure that walks over the outputs of a block, determining the spent-ness.
 */
class BlockHints
{
private:
    std::unordered_set<uint64_t> m_unspent_outputs_index;
    uint64_t m_index{};

public:
    BlockHints(const std::vector<uint64_t>& unspent_offsets);
    bool IsCurrOutputUnspent() { return m_unspent_outputs_index.contains(m_index); };
    void NextOutput() { m_index++; };
};

class Context
{
private:
    std::optional<HintsfileReader> m_hint_reader{};
    bool m_is_starting_from_genesis{};
    bool m_is_complete{};

public:
    /** Aggregate of block inputs and outputs. */
    Aggregate m_aggregate{};
    Context() = default;
    /** Apply the hints from reader to this context. */
    void ApplyHints(HintsfileReader reader);
    /** The entire block history must be aggregated for accelerated IBD. */
    void StartingFromGenesis() { m_is_starting_from_genesis = true; };
    /** Accelerated IBD has completed. */
    void Completed() { m_is_complete = true; };
    /** Accelerated sync is possible when:
     *  1. Blocks are being processed from genesis.
     *  2. A hintfile is present.
     *  3. It has not already been completed.
     */
    bool AcceleratedSyncPossible() { return m_is_starting_from_genesis && m_hint_reader.has_value() && !m_is_complete; };
    /** Read the output spent-ness hints from the file. */
    BlockHints ReadBlockHints(const int& nHeight);
    /** The height to check the aggregate for emptiness. */
    uint32_t StopHeight() { return m_hint_reader->StopHeight(); };
};
} // namespace swiftsync
#endif // BITCOIN_SWIFTSYNC_H
