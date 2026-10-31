// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTXO_SNAPSHOT_H
#define BITCOIN_UTXO_SNAPSHOT_H

#include <coins.h>
#include <consensus/consensus.h>
#include <crypto/muhash.h>
#include <hash.h>
#include <kernel/chainparams.h>
#include <kernel/messagestartchars.h>
#include <serialize.h>
#include <streams.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/blockingqueue.h>
#include <util/byte_units.h>
#include <util/chaintype.h>
#include <util/result.h>
#include <util/signalinterrupt.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <memory>
#include <string>
#include <vector>

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

/** Duplicates (and must stay identical with) kernel/coinstats.cpp TxOutSer. */
inline void WriteCoinStatsElement(DataStream& out, const COutPoint& outpoint, const Coin& coin)
{
    out << outpoint << ((uint32_t{coin.nHeight} << 1) | uint32_t{coin.fCoinBase}) << coin.out;
}

/** Reads a snapshot's coin stream in file order, handing each coin to
 * `handle(outpoint, coin)`. An error from `handle` ends the reading.
 *
 * Errors if txids are out of order, or if the file holds more or fewer
 * coins than the metadata says, or has trailing data. Output indices follow
 * database key order, which may differ from the numeric order required by
 * the commitment.
 *
 * Returns how many coins were read. */
template <typename Handle>
util::Result<uint64_t> ReadSnapshotCoins(AutoFile& file, uint64_t coins_expected, Handle&& handle,
                                         const util::SignalInterrupt* interrupt = nullptr)
{
    uint64_t coins_left{coins_expected};
    uint64_t coins_done{0};

    // last_txid starts at zero, which is below any real txid.
    Txid last_txid;
    try {
        while (coins_left > 0) {
            if (interrupt && *interrupt) {
                return util::Error{Untranslated("Aborting after an interrupt was requested")};
            }

            Txid txid;
            file >> txid;
            if (!(last_txid < txid)) {
                return util::Error{Untranslated(strprintf("Bad snapshot - txids are not in key order: %s follows %s",
                                                          txid.ToString(), last_txid.ToString()))};
            }
            last_txid = txid;

            const uint64_t coins_per_txid{ReadCompactSize(file)};
            if (coins_per_txid > coins_left) {
                return util::Error{Untranslated("Mismatch in coins count in snapshot metadata and actual snapshot data")};
            }

            uint64_t outputs_size{0};
            for (size_t i{0}; i < coins_per_txid; ++i) {
                COutPoint outpoint;
                outpoint.n = static_cast<uint32_t>(ReadCompactSize(file));
                outpoint.hash = txid;
                Coin coin;
                file >> coin;
                // Bound the data retained to sort one transaction for hashing.
                // Its unspent outputs must fit within a valid block's weight.
                outputs_size += GetSerializeSize(coin.out);
                if (outputs_size > MAX_BLOCK_WEIGHT / WITNESS_SCALE_FACTOR) {
                    return util::Error{Untranslated("Bad snapshot - transaction outputs exceed maximum block weight")};
                }
                if (auto res{handle(outpoint, coin)}; !res) {
                    return util::Error{util::ErrorString(res)};
                }
                ++coins_done;
            }
            coins_left -= coins_per_txid;
        }

        // All the promised coins have been read. Reading one more
        // byte should hit EOF, which will be caught below and treated as
        // success because coins_left == 0. If the read succeeds, the file
        // has trailing data.
        std::byte left_over;
        file >> left_over;
        return util::Error{Untranslated(strprintf("Bad snapshot - coins left over after deserializing %d coins", coins_expected))};
    } catch (const std::ios_base::failure&) {
        if (coins_left != 0) {
            return util::Error{Untranslated(strprintf("Bad snapshot format or truncated snapshot after deserializing %d coins", coins_done))};
        }
    }
    return coins_done;
}

/** An entry from the snapshot. The database and hash need different encodings, so
 * it's queued in decoded form, and encoded as needed. */
struct SnapshotItem {
    COutPoint outpoint;
    Coin coin;
};

//! Shared and read-only as the same batch goes to multiple consumers
using SnapshotBatch = std::shared_ptr<const std::vector<SnapshotItem>>;
using SnapshotQueue = util::BlockingQueue<SnapshotBatch>;

/** Maximum number of coins in one batch. */
constexpr size_t SNAPSHOT_ITEMS_PER_BATCH{65536};

/** Maximum number of input bytes (pre-decode) used to populate a
 * batch. Gives a stricter memory boundary for coins that have large
 * scripts. Note the memory usage of the batch will be larger than
 * this. */
constexpr size_t SNAPSHOT_BATCH_BYTES{4_MiB};

/** Reads the coins in `file` and hands them out in batches.
 * `check(outpoint, coin, count)` sees every coin first and provides
 * the ability to abort early. `handle(batch, count)` is called
 * to process batch when either limit is reached, and at the end. */
template <typename Check, typename Handle>
util::Result<void> ReadSnapshotBatches(AutoFile& file, uint64_t coins_expected, Check&& check, Handle&& handle,
                                       const util::SignalInterrupt* interrupt = nullptr)
{
    uint64_t coins_done{0};
    auto batch{std::make_shared<std::vector<SnapshotItem>>()};
    batch->reserve(SNAPSHOT_ITEMS_PER_BATCH);
    int64_t batch_start{file.tell()};
    const auto publish = [&]() {
        if (!batch->empty()) handle(SnapshotBatch{batch}, coins_done);
        batch = std::make_shared<std::vector<SnapshotItem>>();
        batch->reserve(SNAPSHOT_ITEMS_PER_BATCH);
        batch_start = file.tell();
    };

    const auto res{ReadSnapshotCoins(file, coins_expected,
        [&](const COutPoint& outpoint, Coin& coin) -> util::Result<void> {
            if (auto checked{check(outpoint, coin, coins_done)}; !checked) return checked;
            batch->emplace_back(outpoint, std::move(coin));
            ++coins_done;
            if (batch->size() >= SNAPSHOT_ITEMS_PER_BATCH ||
                file.tell() - batch_start >= static_cast<int64_t>(SNAPSHOT_BATCH_BYTES)) {
                publish();
            }
            return {};
        },
        interrupt)};

    if (!batch->empty()) handle(SnapshotBatch{batch}, coins_done);
    if (!res) return util::Error{util::ErrorString(res)};
    return {};
}

/** Calculate hash_serialized_3 of the coins in `queue`, with numerically ordered output indices. */
inline void HashSnapshotBatches(SnapshotQueue& queue, uint256& out)
{
    // The hash object belongs to this thread, and the result is written into the
    // caller's value before the thread ends, so joining it is all the
    // synchronisation the reader needs.
    HashWriter hasher{};
    DataStream element{};
    // The database orders VARINT-encoded output indices lexicographically,
    // whereas the commitment orders them numerically. A transaction may also
    // span batches: retain their storage until all its outputs can be hashed.
    std::vector<const SnapshotItem*> outputs;
    std::vector<SnapshotBatch> retained;
    const auto flush = [&] {
        std::ranges::sort(outputs, {}, [](const SnapshotItem* item) { return item->outpoint.n; });
        for (const auto* item : outputs) {
            element.clear();
            WriteCoinStatsElement(element, item->outpoint, item->coin);
            hasher.write(std::as_bytes(MakeUCharSpan(element)));
        }
        outputs.clear();
        retained.clear();
    };
    SnapshotBatch items;
    while (queue.Pop(items)) {
        for (const SnapshotItem& item : *items) {
            if (!outputs.empty() && outputs.back()->outpoint.hash != item.outpoint.hash) flush();
            outputs.push_back(&item);
        }
        retained.push_back(std::move(items));
    }
    flush();
    out = hasher.GetHash();
}

/** The muhash of the coins in `queue`, added to `accumulator`. MuHash
 * is a commutative hash function, so consumers can keep one accumulator
 * each and combine them at the end. */
inline void MuhashSnapshotBatches(SnapshotQueue& queue, MuHash3072& accumulator)
{
    DataStream element{};
    SnapshotBatch items;
    while (queue.Pop(items)) {
        for (const SnapshotItem& item : *items) {
            element.clear();
            WriteCoinStatsElement(element, item.outpoint, item.coin);
            accumulator.Insert(MakeUCharSpan(element));
        }
    }
}

#endif // BITCOIN_UTXO_SNAPSHOT_H
