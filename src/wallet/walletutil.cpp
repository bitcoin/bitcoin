// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletutil.h>

#include <logging.h>
#include <util/system.h>

fs::path GetWalletDir()
{
    fs::path path;

    if (gArgs.IsArgSet("-walletdir")) {
        path = gArgs.GetArg("-walletdir", "");
        if (!fs::is_directory(path)) {
            // If the path specified doesn't exist, we return the deliberately
            // invalid empty string.
            path = "";
        }
    } else {
        path = GetDataDir();
        // If a wallets directory exists, use that, otherwise default to GetDataDir
        if (fs::is_directory(path / "wallets")) {
            path /= "wallets";
        }
    }

    return path;
}

bool IsBerkeleyBtree(const fs::path& path)
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

std::vector<fs::path> ListWalletDir()
{
    const fs::path wallet_dir = GetWalletDir();
    const fs::path data_dir = GetDataDir();
    const fs::path blocks_dir = GetBlocksDir();

    const size_t offset = wallet_dir.string().size() + 1;
    std::vector<fs::path> paths;
    boost::system::error_code ec;

    // Here we place the top level dirs we want to skip in case walletdir is datadir or blocksdir
    // Those directories are referenced in doc/files.md
    const std::vector<fs::path> ignore_paths = {
                                        blocks_dir,
                                        data_dir / "blktree",
                                        data_dir / "chainstate",
                                        data_dir / "coins",
                                        data_dir / "database",
                                        data_dir / "indexes",
                                        data_dir / "regtest",
                                        data_dir / "signet",
                                        data_dir / "testnet3"
                                        };

    for (auto it = fs::recursive_directory_iterator(wallet_dir, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            LogPrintf("%s: %s %s\n", __func__, ec.message(), it->path().string());
            continue;
        }

        // We don't want to iterate through those special node dirs
        if (std::count(ignore_paths.begin(), ignore_paths.end(), it->path())) {
            it.no_push();
            continue;
        }

        try {
            if (it->status().type() == fs::directory_file && IsBerkeleyBtree(it->path() / "wallet.dat")) {
                // Found a directory which contains wallet.dat btree file, add it as a wallet.
                // Get wallet path relative to walletdir by removing walletdir from the wallet path.
                // This can be replaced by boost::filesystem::lexically_relative once boost is bumped to 1.60.
                paths.emplace_back(it->path().string().substr(offset));
            } else if (it.level() == 0 && it->symlink_status().type() == fs::regular_file && IsBerkeleyBtree(it->path())) {
                if (it->path().filename() == "wallet.dat") {
                    // Found top-level wallet.dat btree file, add top level directory ""
                    // as a wallet.
                    paths.emplace_back();
                } else {
                    // Found top-level btree file not called wallet.dat. Current bitcoin
                    // software will never create these files but will allow them to be
                    // opened in a shared database environment for backwards compatibility.
                    // Add it to the list of available wallets.
                    paths.emplace_back(it->path().filename());
                }
            }
        } catch (const std::exception& e) {
            LogPrintf("%s: Error scanning %s: %s\n", __func__, it->path().string(), e.what());
        }
    }

    return paths;
}
