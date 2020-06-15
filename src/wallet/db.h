// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DB_H
#define BITCOIN_WALLET_DB_H

#include <fs.h>

#include <string>

/** Given a wallet directory path or legacy file path, return path to main data file in the wallet database. */
fs::path WalletDataFilePath(const fs::path& wallet_path);
void SplitWalletPath(const fs::path& wallet_path, fs::path& env_directory, std::string& database_filename);

#endif // BITCOIN_WALLET_DB_H
