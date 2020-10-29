// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletutil.h>

#include <logging.h>
#include <util/system.h>

bool ExistsBerkeleyDatabase(const fs::path& path);
#ifdef USE_SQLITE
bool ExistsSQLiteDatabase(const fs::path& path);
#else
#   define ExistsSQLiteDatabase(path)  (false)
#endif

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

std::vector<fs::path> ListWalletDir()
{
    const fs::path wallet_dir = GetWalletDir();
    std::vector<fs::path> paths;
    boost::system::error_code ec;

    for (auto it = fs::recursive_directory_iterator(wallet_dir, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            LogPrintf("%s: %s %s\n", __func__, ec.message(), it->path().string());
            continue;
        }

        // Get wallet path relative to wallet_dir by removing wallet_dir from the wallet path.
        const fs::path path = it->path().lexically_relative(wallet_dir);

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
    }

    return paths;
}
