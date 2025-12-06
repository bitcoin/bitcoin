// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DUMP_H
#define BITCOIN_WALLET_DUMP_H

#include <util/fs.h>
#include <util/result.h>

#include <string>
#include <vector>

struct bilingual_str;
class ArgsManager;

namespace wallet {
class WalletDatabase;

util::Result<void> DumpWallet(const ArgsManager& args, WalletDatabase& db);
util::Result<void> CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path);
} // namespace wallet

#endif // BITCOIN_WALLET_DUMP_H
