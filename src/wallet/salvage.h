// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SALVAGE_H
#define BITCOIN_WALLET_SALVAGE_H

#include <streams.h>
#include <util/fs.h>

class ArgsManager;
struct bilingual_str;

namespace wallet {
util::Result<void> RecoverDatabaseFile(const ArgsManager& args, const fs::path& file_path);
} // namespace wallet

#endif // BITCOIN_WALLET_SALVAGE_H
