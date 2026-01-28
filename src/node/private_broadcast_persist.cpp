// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/private_broadcast_persist.h>

#include <clientversion.h>
#include <logging.h>
#include <private_broadcast.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/obfuscation.h>
#include <util/syserror.h>

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

using fsbridge::FopenFn;

namespace node {

static const uint64_t PRIVATE_BROADCAST_DUMP_VERSION{1};
static const uint64_t MAX_PRIVATE_BROADCAST_TXS{100000};

void LoadPrivateBroadcast(PrivateBroadcast& pb, const fs::path& load_path, FopenFn mockable_fopen_function)
{
    if (load_path.empty()) return;

    AutoFile file{mockable_fopen_function(load_path, "rb")};
    if (file.IsNull()) {
        LogDebug(BCLog::PRIVBROADCAST, "Failed to open private broadcast file. Continuing anyway.\n");
        return;
    }

    try {
        uint64_t version;
        file >> version;
        if (version != PRIVATE_BROADCAST_DUMP_VERSION) return;

        Obfuscation obfuscation;
        file >> obfuscation;
        file.SetObfuscation(obfuscation);

        uint64_t total_to_load{0};
        file >> total_to_load;
        LogDebug(BCLog::PRIVBROADCAST, "Loading %u private broadcast transactions from file...\n", total_to_load);

        total_to_load = std::min<uint64_t>(total_to_load, MAX_PRIVATE_BROADCAST_TXS);

        for (uint64_t i = 0; i < total_to_load; ++i) {
            CTransactionRef tx;
            file >> TX_WITH_WITNESS(tx);
            pb.Add(tx);
        }
    } catch (const std::exception& e) {
        LogInfo("Failed to deserialize private broadcast data on file: %s. Continuing anyway.\n", e.what());
    }
}

void DumpPrivateBroadcast(const PrivateBroadcast& pb, const fs::path& dump_path, FopenFn mockable_fopen_function, bool skip_file_commit)
{
    const auto infos{pb.GetBroadcastInfo()};
    std::vector<CTransactionRef> txs;
    txs.reserve(infos.size());
    for (const auto& info : infos) txs.push_back(info.tx);

    const fs::path file_fspath{dump_path + ".new"};
    AutoFile file{mockable_fopen_function(file_fspath, "wb")};
    if (file.IsNull()) return;

    try {
        file << PRIVATE_BROADCAST_DUMP_VERSION;

        const Obfuscation obfuscation{FastRandomContext{}.randbytes<Obfuscation::KEY_SIZE>()};
        file << obfuscation;
        file.SetObfuscation(obfuscation);

        file << uint64_t{txs.size()};
        LogDebug(BCLog::PRIVBROADCAST, "Writing %u private broadcast transactions to file...\n", txs.size());
        for (const auto& tx : txs) {
            file << TX_WITH_WITNESS(tx);
        }

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
    } catch (const std::exception& e) {
        LogInfo("Failed to dump private broadcast: %s. Continuing anyway.\n", e.what());
        (void)file.fclose();
    }
}

} // namespace node
