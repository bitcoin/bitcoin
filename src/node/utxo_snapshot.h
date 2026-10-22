// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_UTXO_SNAPSHOT_H
#define BITCOIN_NODE_UTXO_SNAPSHOT_H

#include <kernel/cs_main.h>
#include <sync.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/result.h>

#include <cstdint>
#include <optional>
#include <string_view>

class AutoFile;
class CDBWrapper;
class Chainstate;
struct AssumeutxoData;

namespace util {
class SignalInterrupt;
} // namespace util

namespace node {
//! The file in the snapshot chainstate dir which stores the base blockhash. This is
//! needed to reconstruct snapshot chainstates on init.
//!
//! Because we only allow loading a single snapshot at a time, there will only be one
//! chainstate directory with this filename present within it.
inline const fs::path SNAPSHOT_BLOCKHASH_FILENAME{"base_blockhash"};

//! Write out the blockhash of the snapshot base block that was used to construct
//! this chainstate. This value is read in during subsequent initializations and
//! used to reconstruct snapshot-based chainstates.
bool WriteSnapshotBaseBlockhash(Chainstate& snapshot_chainstate)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! Write out the blockhash of the base block that was used to construct
//! this utxo set snapshot, and sync it. This value is read in during subsequent
//! initializations to recognize this directory as a (completed) snapshot
//! chainstate, so a write that fails removes the file again rather than leave the
//! directory looking complete.
bool WriteSnapshotBaseBlockhash(const fs::path& datadir, const uint256& base_blockhash)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! Read the blockhash of the snapshot base block that was used to construct the
//! chainstate.
std::optional<uint256> ReadSnapshotBaseBlockhash(fs::path chaindir)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! Suffix appended to the chainstate (leveldb) dir when created based upon
//! a snapshot.
inline constexpr std::string_view SNAPSHOT_CHAINSTATE_SUFFIX = "_snapshot";


//! Return a path to the snapshot-based chainstate dir, if one exists.
std::optional<fs::path> FindAssumeutxoChainstateDir(const fs::path& data_dir);

//! Write the coins from `file` into `db`, checking the coins
//! hash_serialized_3 against au_data, and writing the best block. The
//! caller owns `db` and must discard it if there is an error.
util::Result<void> WriteSnapshotCoins(AutoFile& file, uint64_t coins_expected,
                                      const uint256& base_blockhash, int base_height,
                                      const AssumeutxoData& au_data, CDBWrapper& db,
                                      const util::SignalInterrupt& interrupt);

//! Build the snapshot chainstate database under `datadir` and sync the files it
//! wrote. The directory is not a chainstate until WriteSnapshotBaseBlockhash()
//! marks it complete, and the caller has to remove it if this fails, after the
//! database has been closed.
//!
//! Refuses to build if the directory already exists: one left behind by an
//! interrupted load has coins in it that the snapshot does not, and they would
//! survive into the set.
util::Result<void> WriteSnapshotChainstate(const fs::path& datadir, AutoFile& coins_file,
                                           uint64_t coins_expected, const uint256& base_blockhash,
                                           int base_height, const AssumeutxoData& au_data,
                                           const util::SignalInterrupt& interrupt);

} // namespace node

#endif // BITCOIN_NODE_UTXO_SNAPSHOT_H
