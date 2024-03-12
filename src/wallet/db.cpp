// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <logging.h>
#include <util/fs.h>
#include <wallet/db.h>

#include <exception>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace wallet {
bool operator<(BytePrefix a, Span<const std::byte> b) { return a.prefix < b.subspan(0, std::min(a.prefix.size(), b.size())); }
bool operator<(Span<const std::byte> a, BytePrefix b) { return a.subspan(0, std::min(a.size(), b.prefix.size())) < b.prefix; }

std::vector<fs::path> ListDatabases(const fs::path& wallet_dir)
{
    std::vector<fs::path> paths;
    std::error_code ec;

    for (auto it = fs::recursive_directory_iterator(wallet_dir, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            if (fs::is_directory(*it)) {
                it.disable_recursion_pending();
                LogInfo("%s: %s %s -- skipping.\n", __func__, ec.message(), fs::PathToString(it->path()));
            } else {
                LogInfo("%s: %s %s\n", __func__, ec.message(), fs::PathToString(it->path()));
            }
            continue;
        }

        try {
            const fs::path path{it->path().lexically_relative(wallet_dir)};

            if (it->status().type() == fs::file_type::directory &&
                (IsBDBFile(BDBDataFile(it->path())) || IsSQLiteFile(SQLiteDataFile(it->path())))) {
                // Found a directory which contains wallet.dat btree file, add it as a wallet.
                paths.emplace_back(path);
            } else if (it.depth() == 0 && it->symlink_status().type() == fs::file_type::regular && IsBDBFile(it->path())) {
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
            LogInfo("%s: Error scanning %s: %s\n", __func__, fs::PathToString(it->path()), e.what());
            it.disable_recursion_pending();
        }
    }

    return paths;
}

fs::path BDBDataFile(const fs::path& wallet_path)
{
    if (fs::is_regular_file(wallet_path)) {
        // Special case for backwards compatibility: if wallet path points to an
        // existing file, treat it as the path to a BDB data file in a parent
        // directory that also contains BDB log files.
        return wallet_path;
    } else {
        // Normal case: Interpret wallet path as a directory path containing
        // data and log files.
        return wallet_path / "wallet.dat";
    }
}

fs::path SQLiteDataFile(const fs::path& path)
{
    return path / "wallet.dat";
}

bool IsBDBFile(const fs::path& path)
{
    if (!fs::exists(path)) return false;

    // A Berkeley DB Btree file has at least 4K.
    // This check also prevents opening lock files.
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) LogInfo("%s: %s %s\n", __func__, ec.message(), fs::PathToString(path));
    if (size < 4096) return false;

    std::ifstream file{path, std::ios::binary};
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
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) LogInfo("%s: %s %s\n", __func__, ec.message(), fs::PathToString(path));
    if (size < 512) return false;

    std::ifstream file{path, std::ios::binary};
    if (!file.is_open()) return false;

    // Magic is at beginning and is 16 bytes long
    char magic[16];
    file.read(magic, 16);

    // Application id is at offset 68 and 4 bytes long
    file.seekg(68, std::ios::beg);
    char app_id[4];
    file.read(app_id, 4);

    file.close();

    // Check the magic, see https://sqlite.org/fileformat.html
    std::string magic_str(magic, 16);
    if (magic_str != std::string{"SQLite format 3\000", 16}) {
        return false;
    }

    // Check the application id matches our network magic
    return memcmp(Params().MessageStart().data(), app_id, 4) == 0;
}

void ReadDatabaseArgs(const ArgsManager& args, DatabaseOptions& options)
{
    // Override current options with args values, if any were specified
    options.use_unsafe_sync = args.GetBoolArg("-unsafesqlitesync", options.use_unsafe_sync);
    options.use_shared_memory = !args.GetBoolArg("-privdb", !options.use_shared_memory);
    options.max_log_mb = args.GetIntArg("-dblogsize", options.max_log_mb);
}

} // namespace wallet
