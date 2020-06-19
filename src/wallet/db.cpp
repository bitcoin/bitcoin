// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <wallet/db.h>

#include <string>

void SplitWalletPath(const fs::path& wallet_path, fs::path& env_directory, std::string& database_filename)
{
    if (fs::is_regular_file(wallet_path)) {
        // Special case for backwards compatibility: if wallet path points to an
        // existing file, treat it as the path to a BDB data file in a parent
        // directory that also contains BDB log files.
        env_directory = wallet_path.parent_path();
        database_filename = wallet_path.filename().string();
    } else {
        // Normal case: Interpret wallet path as a directory path containing
        // data and log files.
        env_directory = wallet_path;
        database_filename = "wallet.dat";
    }
}

fs::path WalletDataFilePath(const fs::path& wallet_path)
{
    fs::path env_directory;
    std::string database_filename;
    SplitWalletPath(wallet_path, env_directory, database_filename);
    return env_directory / database_filename;
}
