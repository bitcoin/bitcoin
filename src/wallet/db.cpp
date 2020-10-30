// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <fs.h>
#include <logging.h>
#include <wallet/db.h>

#include <string>

fs::path GetWalletDir();

#ifdef USE_BDB
bool ExistsBerkeleyDatabase(const fs::path& path);
#else
#   define ExistsBerkeleyDatabase(path)  (false)
#endif
#ifdef USE_SQLITE
bool ExistsSQLiteDatabase(const fs::path& path);
#else
#   define ExistsSQLiteDatabase(path)  (false)
#endif

std::vector<fs::path> ListWalletDir()
{
    const fs::path wallet_dir = GetWalletDir();
    const size_t offset = wallet_dir.string().size() + 1;
    std::vector<fs::path> paths;
    boost::system::error_code ec;

    for (auto it = fs::recursive_directory_iterator(wallet_dir, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            LogPrintf("%s: %s %s\n", __func__, ec.message(), it->path().string());
            continue;
        }

        try {
            // Get wallet path relative to walletdir by removing walletdir from the wallet path.
            // This can be replaced by boost::filesystem::lexically_relative once boost is bumped to 1.60.
            const fs::path path = it->path().string().substr(offset);

            if (it->status().type() == fs::directory_file &&
                (ExistsBerkeleyDatabase(it->path()) || ExistsSQLiteDatabase(it->path()))) {
                // Found a directory which contains wallet.dat btree file, add it as a wallet.
                paths.emplace_back(path);
            } else if (it.level() == 0 && it->symlink_status().type() == fs::regular_file && ExistsBerkeleyDatabase(it->path())) {
                if (it->path().filename() == "wallet.dat") {
                    // Found top-level wallet.dat btree file, add top level directory ""
                    // as a wallet.
                    paths.emplace_back();
                } else {
                    // Found top-level btree file not called wallet.dat. Current bitcoin
                    // software will never create these files but will allow them to be
                    // opened in a shared database environment for backwards compatibility.
                    // Add it to the list of available wallets.
                    paths.emplace_back(path);
                }
            }
        } catch (const std::exception& e) {
            LogPrintf("%s: Error scanning %s: %s\n", __func__, it->path().string(), e.what());
            it.no_push();
        }
    }

    return paths;
}

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

bool IsBDBFile(const fs::path& path)
{
    if (!fs::exists(path)) return false;

    // A Berkeley DB Btree file has at least 4K.
    // This check also prevents opening lock files.
    boost::system::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) LogPrintf("%s: %s %s\n", __func__, ec.message(), path.string());
    if (size < 4096) return false;

    fsbridge::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(12, std::ios::beg); // Magic bytes start at offset 12
    uint32_t data = 0;
    file.read((char*) &data, sizeof(data)); // Read 4 bytes of file to compare against magic

    // Berkeley DB Btree magic bytes, from:
    //  https://github.com/file/file/blob/5824af38469ec1ca9ac3ffd251e7afe9dc11e227/magic/Magdir/database#L74-L75
    //  - big endian systems - 00 05 31 62
    //  - little endian systems - 62 31 05 00
    return data == 0x00053162 || data == 0x62310500;
}

bool IsSQLiteFile(const fs::path& path)
{
    if (!fs::exists(path)) return false;

    // A SQLite Database file is at least 512 bytes.
    boost::system::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) LogPrintf("%s: %s %s\n", __func__, ec.message(), path.string());
    if (size < 512) return false;

    fsbridge::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    // Magic is at beginning and is 16 bytes long
    char magic[16];
    file.read(magic, 16);

    // Application id is at offset 68 and 4 bytes long
    file.seekg(68, std::ios::beg);
    char app_id[4];
    file.read(app_id, 4);

    file.close();

    // Check the magic, see https://sqlite.org/fileformat2.html
    std::string magic_str(magic, 16);
    if (magic_str != std::string("SQLite format 3", 16)) {
        return false;
    }

    // Check the application id matches our network magic
    return memcmp(Params().MessageStart(), app_id, 4) == 0;
}
