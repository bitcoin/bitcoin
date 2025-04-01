// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_persist.h>

#include <clientversion.h>
#include <consensus/amount.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/obfuscation.h>
#include <util/signalinterrupt.h>
#include <util/syserror.h>
#include <util/time.h>
#include <validation.h>

#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

using fsbridge::FopenFn;

namespace node {

static const uint64_t MEMPOOL_DUMP_VERSION_NO_XOR_KEY{1};
static const uint64_t MEMPOOL_DUMP_VERSION{2};

bool LoadMempool(CTxMemPool& pool, const fs::path& load_path, Chainstate& active_chainstate, ImportMempoolOptions&& opts)
{
    if (load_path.empty()) return false;

    AutoFile file{opts.mockable_fopen_function(load_path, "rb")};
    if (file.IsNull()) {
        LogInfo("Failed to open mempool file. Continuing anyway.\n");
        return false;
    }

    int64_t count = 0;
    int64_t expired = 0;
    int64_t failed = 0;
    int64_t already_there = 0;
    int64_t unbroadcast = 0;
    const auto now{NodeClock::now()};

    try {
        uint64_t version;
        file >> version;

        if (version == MEMPOOL_DUMP_VERSION_NO_XOR_KEY) {
            file.SetObfuscation({});
        } else if (version == MEMPOOL_DUMP_VERSION) {
            Obfuscation obfuscation;
            file >> obfuscation;
            file.SetObfuscation(obfuscation);
        } else {
            return false;
        }

        uint64_t total_txns_to_load;
        file >> total_txns_to_load;
        uint64_t txns_tried = 0;
        LogInfo("Loading %u mempool transactions from file...\n", total_txns_to_load);
        int next_tenth_to_report = 0;
        while (txns_tried < total_txns_to_load) {
            const int percentage_done(100.0 * txns_tried / total_txns_to_load);
            if (next_tenth_to_report < percentage_done / 10) {
                LogInfo("Progress loading mempool transactions from file: %d%% (tried %u, %u remaining)\n",
                        percentage_done, txns_tried, total_txns_to_load - txns_tried);
                next_tenth_to_report = percentage_done / 10;
            }
            ++txns_tried;

            CTransactionRef tx;
            int64_t nTime;
            int64_t nFeeDelta;
            file >> TX_WITH_WITNESS(tx);
            file >> nTime;
            file >> nFeeDelta;

            if (opts.use_current_time) {
                nTime = TicksSinceEpoch<std::chrono::seconds>(now);
            }

            CAmount amountdelta = nFeeDelta;
            if (amountdelta && opts.apply_fee_delta_priority) {
                pool.PrioritiseTransaction(tx->GetHash(), amountdelta);
            }
            if (nTime > TicksSinceEpoch<std::chrono::seconds>(now - pool.m_opts.expiry)) {
                LOCK(cs_main);
                const auto& accepted = AcceptToMemoryPool(active_chainstate, tx, nTime, /*bypass_limits=*/false, /*test_accept=*/false);
                if (accepted.m_result_type == MempoolAcceptResult::ResultType::VALID) {
                    ++count;
                } else {
                    // mempool may contain the transaction already, e.g. from
                    // wallet(s) having loaded it while we were processing
                    // mempool transactions; consider these as valid, instead of
                    // failed, but mark them as 'already there'
                    if (pool.exists(tx->GetHash())) {
                        ++already_there;
                    } else {
                        ++failed;
                    }
                }
            } else {
                ++expired;
            }
            if (active_chainstate.m_chainman.m_interrupt)
                return false;
        }
        std::map<Txid, CAmount> mapDeltas;
        file >> mapDeltas;

        if (opts.apply_fee_delta_priority) {
            for (const auto& i : mapDeltas) {
                pool.PrioritiseTransaction(i.first, i.second);
            }
        }

        std::set<Txid> unbroadcast_txids;
        file >> unbroadcast_txids;
        if (opts.apply_unbroadcast_set) {
            unbroadcast = unbroadcast_txids.size();
            for (const auto& txid : unbroadcast_txids) {
                // Ensure transactions were accepted to mempool then add to
                // unbroadcast set.
                if (pool.get(txid) != nullptr) pool.AddUnbroadcastTx(txid);
            }
        }
    } catch (const std::exception& e) {
        LogInfo("Failed to deserialize mempool data on file: %s. Continuing anyway.\n", e.what());
        return false;
    }

    LogInfo("Imported mempool transactions from file: %i succeeded, %i failed, %i expired, %i already there, %i waiting for initial broadcast\n", count, failed, expired, already_there, unbroadcast);
    return true;
}

bool DumpMempool(const CTxMemPool& pool, const fs::path& dump_path, FopenFn mockable_fopen_function, bool skip_file_commit)
{
    auto start = SteadyClock::now();

    std::map<Txid, CAmount> mapDeltas;
    std::vector<TxMempoolInfo> vinfo;
    std::set<Txid> unbroadcast_txids;

    static Mutex dump_mutex;
    LOCK(dump_mutex);

    {
        LOCK(pool.cs);
        for (const auto &i : pool.mapDeltas) {
            mapDeltas[i.first] = i.second;
        }
        vinfo = pool.infoAll();
        unbroadcast_txids = pool.GetUnbroadcastTxs();
    }

    auto mid = SteadyClock::now();

    const fs::path file_fspath{dump_path + ".new"};
    AutoFile file{mockable_fopen_function(file_fspath, "wb")};
    if (file.IsNull()) {
        return false;
    }

    try {
        const uint64_t version{pool.m_opts.persist_v1_dat ? MEMPOOL_DUMP_VERSION_NO_XOR_KEY : MEMPOOL_DUMP_VERSION};
        file << version;

        if (!pool.m_opts.persist_v1_dat) {
            const Obfuscation obfuscation{FastRandomContext{}.randbytes<Obfuscation::KEY_SIZE>()};
            file << obfuscation;
            file.SetObfuscation(obfuscation);
        } else {
            file.SetObfuscation({});
        }

        uint64_t mempool_transactions_to_write(vinfo.size());
        file << mempool_transactions_to_write;
        LogInfo("Writing %u mempool transactions to file...\n", mempool_transactions_to_write);
        for (const auto& i : vinfo) {
            file << TX_WITH_WITNESS(*(i.tx));
            file << int64_t{count_seconds(i.m_time)};
            file << int64_t{i.nFeeDelta};
            mapDeltas.erase(i.tx->GetHash());
        }

        file << mapDeltas;

        LogInfo("Writing %d unbroadcast transactions to file.\n", unbroadcast_txids.size());
        file << unbroadcast_txids;

        if (!skip_file_commit && !file.Commit()) {
            (void)file.fclose();
            throw std::runtime_error("Commit failed");
        }
        if (file.fclose() != 0) {
            throw std::runtime_error(
                strprintf("Error closing %s: %s", fs::PathToString(file_fspath), SysErrorString(errno)));
        }
        if (!RenameOver(dump_path + ".new", dump_path)) {
            throw std::runtime_error("Rename failed");
        }
        auto last = SteadyClock::now();

        LogInfo("Dumped mempool: %.3fs to copy, %.3fs to dump, %d bytes dumped to file\n",
                  Ticks<SecondsDouble>(mid - start),
                  Ticks<SecondsDouble>(last - mid),
                  fs::file_size(dump_path));
    } catch (const std::exception& e) {
        LogInfo("Failed to dump mempool: %s. Continuing anyway.\n", e.what());
        (void)file.fclose();
        return false;
    }
    return true;
}

} // namespace node
