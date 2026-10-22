// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/utxo_snapshot.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/amount.h>
#include <dbwrapper.h>
#include <kernel/caches.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <txdb.h>
#include <uint256.h>
#include <util/blockingqueue.h>
#include <util/byte_units.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/log.h>
#include <util/result.h>
#include <util/thread.h>
#include <util/translation.h>
#include <utxo_snapshot.h>
#include <validation.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

/** How often, in coins, progress is reported while a snapshot is being
 * loaded. */
static constexpr uint64_t SNAPSHOT_PROGRESS_INTERVAL{10'000'000};

/** How many batches each of the reader's queues may hold. */
static constexpr size_t SNAPSHOT_QUEUE_BATCHES{16};

/** The size of each batch sent to the leveldb writer, and the number
 * of batches that may be in the queue. Does not impact how frequently
 * writes get flushed by the writer. */
static constexpr size_t SNAPSHOT_DATA_BATCH_BYTES{4_MiB};
static constexpr size_t SNAPSHOT_DB_BATCHES{8};

namespace node {

bool WriteSnapshotBaseBlockhash(Chainstate& snapshot_chainstate)
{
    AssertLockHeld(::cs_main);
    assert(snapshot_chainstate.m_from_snapshot_blockhash);

    const std::optional<fs::path> chaindir = snapshot_chainstate.StoragePath();
    assert(chaindir); // Sanity check that chainstate isn't in-memory.
    const fs::path write_to = *chaindir / node::SNAPSHOT_BLOCKHASH_FILENAME;

    FILE* file{fsbridge::fopen(write_to, "wb")};
    AutoFile afile{file};
    if (afile.IsNull()) {
        LogError("[snapshot] failed to open base blockhash file for writing: %s",
                  fs::PathToString(write_to));
        return false;
    }
    afile << *snapshot_chainstate.m_from_snapshot_blockhash;

    if (afile.fclose() != 0) {
        LogError("[snapshot] failed to close base blockhash file %s after writing",
                  fs::PathToString(write_to));
        return false;
    }
    return true;
}

bool WriteSnapshotBaseBlockhash(const fs::path& datadir, const uint256& base_blockhash)
{
    const fs::path chainstate_dir{
        datadir / fs::u8path(strprintf("chainstate%s", std::string{SNAPSHOT_CHAINSTATE_SUFFIX}))};
    const fs::path write_to{chainstate_dir / SNAPSHOT_BLOCKHASH_FILENAME};

    FILE* file{fsbridge::fopen(write_to, "wb")};
    AutoFile afile{file};
    if (afile.IsNull()) {
        LogError("[snapshot] failed to open base blockhash file for writing: %s",
                  fs::PathToString(write_to));
        return false;
    }
    afile << base_blockhash;

    bool success = true;
    if (!afile.Commit()) {
        success = false;
        LogError("[snapshot] failed to sync base blockhash file %s after writing",
                  fs::PathToString(write_to));
    }
    if (afile.fclose() != 0) {
        success = false;
        LogError("[snapshot] failed to close base blockhash file %s after writing",
                  fs::PathToString(write_to));
    }
    if (!success) {
        // Failure to commit or sync should leave nothing behind: otherwise
        // there is a risk that the next startup will still see a corrupt file,
        // despite the failure report.
        try {
            fs::remove(write_to);
            DirectoryCommit(chainstate_dir);
        } catch (const fs::filesystem_error& e) {
            LogError("[snapshot] failed to remove %s: %s. The snapshot chainstate "
                      "directory may be read as complete at the next startup.",
                      fs::PathToString(write_to), e.code().message());
        }
    } else {
        DirectoryCommit(chainstate_dir);
    }
    return success;
}

std::optional<uint256> ReadSnapshotBaseBlockhash(fs::path chaindir)
{
    if (!fs::exists(chaindir)) {
        LogWarning("[snapshot] cannot read base blockhash: no chainstate dir "
            "exists at path %s", fs::PathToString(chaindir));
        return std::nullopt;
    }
    const fs::path read_from = chaindir / node::SNAPSHOT_BLOCKHASH_FILENAME;
    const std::string read_from_str = fs::PathToString(read_from);

    if (!fs::exists(read_from)) {
        LogWarning("[snapshot] snapshot chainstate dir is malformed! no base blockhash file "
            "exists at path %s. Try deleting %s and calling loadtxoutset again?",
            fs::PathToString(chaindir), read_from_str);
        return std::nullopt;
    }

    uint256 base_blockhash;
    FILE* file{fsbridge::fopen(read_from, "rb")};
    AutoFile afile{file};
    if (afile.IsNull()) {
        LogWarning("[snapshot] failed to open base blockhash file for reading: %s",
            read_from_str);
        return std::nullopt;
    }
    afile >> base_blockhash;

    int64_t position = afile.tell();
    afile.seek(0, SEEK_END);
    if (position != afile.tell()) {
        LogWarning("[snapshot] unexpected trailing data in %s", read_from_str);
    }
    return base_blockhash;
}

std::optional<fs::path> FindAssumeutxoChainstateDir(const fs::path& data_dir)
{
    fs::path possible_dir =
        data_dir / fs::u8path(strprintf("chainstate%s", SNAPSHOT_CHAINSTATE_SUFFIX));

    if (fs::exists(possible_dir)) {
        return possible_dir;
    }
    return std::nullopt;
}

namespace {
using DBBatchQueue = util::BlockingQueue<std::vector<std::byte>>;

/** Serializes coins into the key/value format used within the chainstate
 * leveldb, though without the obfuscation step. An exception leaving
 * this thread would call std::terminate, so a failure is stored
 * instead. Finish unblocks whichever side is waiting. */
void SnapshotSerializeTask(SnapshotQueue& in, DBBatchQueue& out, std::optional<bilingual_str>& error)
{
    try {
        RawCoinDataDBWriter::Batch batch{SNAPSHOT_DATA_BATCH_BYTES};
        SnapshotBatch items;
        while (in.Pop(items)) {
            for (const SnapshotItem& item : *items) {
                if (auto to_send = batch.Add(item.outpoint, item.coin); !to_send.empty()) {
                    out.Push(std::move(to_send));
                }
            }
        }
        if (!batch.empty()) out.Push(batch.Take());
    } catch (const std::exception& e) {
        in.Finish();
        out.Finish();
        error = Untranslated(strprintf("Failed to serialize snapshot coins: %s", e.what()));
    }
}

/** Writes serialized records to leveldb. Only this thread touches `db` until joined. */
void SnapshotWriteTask(DBBatchQueue& in, CDBWrapper& db, std::optional<bilingual_str>& error)
{
    try {
        RawCoinDataDBWriter writer{db, DEFAULT_DB_CACHE_BATCH}; // always dump snapshots at default batch size
        std::vector<std::byte> batch;
        while (in.Pop(batch)) writer.Write(batch);
        writer.Flush();
    } catch (const std::exception& e) {
        in.Finish();
        error = Untranslated(strprintf("Failed to write snapshot coins: %s", e.what()));
    }
}

util::Result<void> CheckSnapshotCommitment(const uint256& hash_serialized, const AssumeutxoData& au_data)
{
    if (AssumeutxoHash{hash_serialized} != au_data.hash_serialized) {
        return util::Error{Untranslated(strprintf("Bad snapshot content hash: expected %s, got %s",
            au_data.hash_serialized.ToString(), hash_serialized.ToString()))};
    }
    return {};
}

} // namespace

util::Result<void> WriteSnapshotCoins(AutoFile& file, uint64_t coins_expected,
                                      const uint256& base_blockhash, int base_height,
                                      const AssumeutxoData& au_data, CDBWrapper& db,
                                      const util::SignalInterrupt& interrupt)
{
    LogInfo("[snapshot] loading %d coins from snapshot %s", coins_expected, base_blockhash.ToString());

    std::optional<bilingual_str> reader_error{};
    std::optional<bilingual_str> serialize_error{};
    std::optional<bilingual_str> writer_error{};
    uint256 hash_serialized{};

    SnapshotQueue hash_queue{SNAPSHOT_QUEUE_BATCHES};
    SnapshotQueue serialize_queue{SNAPSHOT_QUEUE_BATCHES};
    DBBatchQueue write_queue{SNAPSHOT_DB_BATCHES};
    uint64_t coins_reported{0};
    std::thread reader_thread{util::TraceThread, "utxoread", [&] {
        auto res{ReadSnapshotBatches(file, coins_expected,
            [&](const COutPoint& outpoint, Coin& coin, uint64_t coins_done) -> util::Result<void> {
                // Additional checks beyond parsing: coin height
                // shouldn't be in the future, and the amount should
                // be in range.
                if (coin.nHeight > base_height ||
                    outpoint.n >= std::numeric_limits<decltype(outpoint.n)>::max()) { // avoid wrap-around in ApplyHash
                    return util::Error{Untranslated(strprintf("Bad snapshot data after deserializing %d coins", coins_done))};
                }
                if (!MoneyRange(coin.out.nValue)) {
                    return util::Error{Untranslated(strprintf("Bad snapshot data after deserializing %d coins - bad tx out value", coins_done))};
                }
                return {};
            },
            [&](SnapshotBatch batch, uint64_t coins_done) {
                hash_queue.Push(batch);
                serialize_queue.Push(batch);
                if (coins_done - coins_reported >= SNAPSHOT_PROGRESS_INTERVAL) {
                    coins_reported = coins_done;
                    LogInfo("[snapshot] %d coins read (%.2f%%)",
                            coins_done, 100.0 * coins_done / coins_expected);
                }
            },
            &interrupt)};
        if (!res) reader_error = util::ErrorString(res);
    }};
    std::thread hash_thread{util::TraceThread, "utxohash", [&] {
        HashSnapshotBatches(hash_queue, hash_serialized);
    }};
    std::thread serialize_thread{util::TraceThread, "utxoser", [&] {
        SnapshotSerializeTask(serialize_queue, write_queue, serialize_error);
    }};
    std::thread writer_thread{util::TraceThread, "utxowrite", [&] {
        SnapshotWriteTask(write_queue, db, writer_error);
    }};

    // Shut down in pipeline order: after joining each thread, finish
    // the queues it was producer for to ensure consumers also end before
    // joining them.
    reader_thread.join();
    hash_queue.Finish();
    serialize_queue.Finish();
    hash_thread.join();
    serialize_thread.join();
    write_queue.Finish();
    writer_thread.join();

    if (reader_error) return util::Error{*reader_error};
    if (serialize_error) return util::Error{*serialize_error};
    if (writer_error) return util::Error{*writer_error};

    if (auto commit_result = CheckSnapshotCommitment(hash_serialized, au_data); !commit_result) {
        return commit_result;
    }

    RawCoinDataDBWriter writer{db, DEFAULT_DB_CACHE_BATCH};
    writer.WriteBestBlock(base_blockhash);
    return {};
}

util::Result<void> WriteSnapshotChainstate(const fs::path& datadir, AutoFile& coins_file,
                                           uint64_t coins_expected, const uint256& base_blockhash,
                                           int base_height, const AssumeutxoData& au_data,
                                           const util::SignalInterrupt& interrupt)
{
    const fs::path chainstate_dir{
        datadir / fs::u8path(strprintf("chainstate%s", std::string{SNAPSHOT_CHAINSTATE_SUFFIX}))};

    // The base blockhash file is written last and is what makes a directory a
    // chainstate, so one that exists is either an active chainstate or what an
    // interrupted load left behind; the coins already in it would end up in the
    // set along with the ones being written.
    if (fs::exists(chainstate_dir)) {
        return util::Error{Untranslated(strprintf("Snapshot chainstate directory %s already exists. "
                                                  "Delete it and call loadtxoutset again.",
                                                  fs::PathToString(chainstate_dir)))};
    }

    {
        // Use the default cache size so snapshot loads are predictable.
        CDBWrapper db{DBParams{
            .path = chainstate_dir,
            .cache_bytes = DEFAULT_DB_CACHE_BATCH,
            .obfuscate = true,
            .nosync = true,
        }};
        if (auto written{WriteSnapshotCoins(coins_file, coins_expected, base_blockhash, base_height, au_data, db, interrupt)}; !written) {
            return written;
        }
    }

    // The coins went in without syncing, so they have to be committed before the base
    // blockhash file can be written.
    if (!DirectoryFilesCommit(chainstate_dir)) {
        return util::Error{Untranslated(strprintf("Could not sync %s", fs::PathToString(chainstate_dir)))};
    }
    return {};
}

} // namespace node
