// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <util/fs.h>
#include <util/log.h>
#include <util/translation.h>
#include <wallet/db.h>

#include <algorithm>
#include <exception>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace wallet {
bool operator<(BytePrefix a, std::span<const std::byte> b) { return std::ranges::lexicographical_compare(a.prefix, b.subspan(0, std::min(a.prefix.size(), b.size()))); }
bool operator<(std::span<const std::byte> a, BytePrefix b) { return std::ranges::lexicographical_compare(a.subspan(0, std::min(a.size(), b.prefix.size())), b.prefix); }

std::vector<std::pair<fs::path, std::string>> ListDatabases(const fs::path& wallet_dir)
{
    std::vector<std::pair<fs::path, std::string>> paths;
    std::error_code ec;

    for (auto it = fs::recursive_directory_iterator(wallet_dir, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        assert(!ec); // Loop should exit on error.
        try {
            const fs::path path{it->path().lexically_relative(wallet_dir)};

            if (it->status().type() == fs::file_type::directory) {
                if (IsBDBFile(BDBDataFile(it->path()))) {
                    // Found a directory which contains wallet.dat btree file, add it as a wallet with BERKELEY format.
                    paths.emplace_back(path, "bdb");
                } else if (IsSQLiteFile(SQLiteDataFile(it->path()))) {
                    // Found a directory which contains wallet.dat sqlite file, add it as a wallet with SQLITE format.
                    paths.emplace_back(path, "sqlite");
                }
            } else if (it.depth() == 0 && it->symlink_status().type() == fs::file_type::regular && it->path().extension() != ".bak") {
                if (it->path().filename() == "wallet.dat") {
                    // Found top-level wallet.dat file, add top level directory ""
                    // as a wallet.
                    if (IsBDBFile(it->path())) {
                        paths.emplace_back(fs::path(), "bdb");
                    } else if (IsSQLiteFile(it->path())) {
                        paths.emplace_back(fs::path(), "sqlite");
                    }
                } else if (IsBDBFile(it->path())) {
                    // Found top-level btree file not called wallet.dat. Current bitcoin
                    // software will never create these files but will allow them to be
                    // opened in a shared database environment for backwards compatibility.
                    // Add it to the list of available wallets.
                    paths.emplace_back(path, "bdb");
                }
            }
        } catch (const std::exception& e) {
            LogWarning("Error while scanning wallet dir item: %s [%s].", e.what(), fs::PathToString(it->path()));
            it.disable_recursion_pending();
        }
    }
    if (ec) {
        // Loop could have exited with an error due to one of:
        // * wallet_dir itself not being scannable.
        // * increment() failure. (Observed on Windows native builds when
        //   removing the ACL read permissions of a wallet directory after the
        //   process started).
        LogWarning("Error scanning directory entries under %s: %s", fs::PathToString(wallet_dir), ec.message());
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

static bilingual_str FileError(const fs::path& path, const std::error_code& ec)
{
    bilingual_str error{strprintf(_("Error: Wallet db file cannot be modified. Path: %s, Error: %s."), fs::PathToString(path), ec.message())};
    if (ec == std::errc::permission_denied || ec == std::errc::operation_not_permitted || ec == std::errc::read_only_file_system) {
        error += Untranslated(" ") + _("Adjust directory or file permissions and try again.");
    }
    return error;
}

util::Result<void> WalletDatabase::Erase()
{
    Close();
    for (const fs::path& file : Files()) {
        std::error_code ec;
        fs::remove(file, ec);
        if (ec) return util::Error{FileError(file, ec)};
    }
    return {};
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
    if (ec) LogWarning("Error reading file_size: %s [%s]", ec.message(), fs::PathToString(path));
    if (size < 4096) return false;

    std::ifstream file{path.std_path(), std::ios::binary};
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
    if (ec) LogWarning("Error reading file_size: %s [%s]", ec.message(), fs::PathToString(path));
    if (size < 512) return false;

    std::ifstream file{path.std_path(), std::ios::binary};
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
}

util::Result<void> MoveDatabase(std::unique_ptr<WalletDatabase>& db, const fs::path& dst_path, DatabaseOptions& opts)
{
    const fs::path wallet_file{fs::PathFromString(db->Filename())};
    const std::vector<fs::path> files{db->Files()};
    db->Close();

    const fs::path dst_file{dst_path / fs::path{wallet_file.filename()}};

    // Other files of this db should never be present at the destination, but an interrupted earlier migration or an
    // outside process may leave them there. Return an error if one looks like a wallet database, delete it otherwise.
    // A leftover journal is the main case, as SQLite would roll it back into the moved db, corrupting it.
    std::error_code ec;
    for (const fs::path& file : files) {
        const fs::path leftover{dst_path / fs::path{file.filename()}};
        if (leftover == dst_file) continue; // Replaced atomically by the rename below
        if (IsBDBFile(leftover) || IsSQLiteFile(leftover)) {
            return util::Error{strprintf(_("Error: Unable to move the wallet db, %s is a wallet database."), fs::PathToString(leftover))};
        }
        if (fs::remove(leftover, ec)) LogInfo("Removed db leftover during move: %s", fs::PathToString(leftover));
        if (ec) return util::Error{FileError(leftover, ec)};
    }

    fs::create_directories(dst_path, ec);
    if (ec) return util::Error{strprintf(_("Error: Unable to create wallet directory %s: %s."), fs::PathToString(dst_path), ec.message())};
    fs::rename(wallet_file, dst_file, ec);
    if (ec) return util::Error{strprintf(_("Error: Unable to move the new wallet db from %s to %s: %s."), fs::PathToString(wallet_file), fs::PathToString(dst_file), ec.message())};

    // Reload db connection
    DatabaseStatus db_status;
    opts.require_create = false;
    opts.require_existing = true;
    bilingual_str error;
    std::unique_ptr<WalletDatabase> moved_db{MakeDatabase(dst_path, opts, db_status, error)};
    if (!moved_db) {
        fs::remove(dst_file, ec); // Don't leave a db that can't be opened at the destination
        return util::Error{error};
    }
    db = std::move(moved_db);
    return {};
}

} // namespace wallet
