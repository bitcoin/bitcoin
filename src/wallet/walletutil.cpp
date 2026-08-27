// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletutil.h>

#include <chainparams.h>
#include <common/args.h>
#include <key_io.h>
#include <util/log.h>

namespace wallet {
fs::path GetWalletDir()
{
    fs::path path;

    if (gArgs.IsArgSet("-walletdir")) {
        path = gArgs.GetPathArg("-walletdir");
        if (!fs::is_directory(path)) {
            // If the path specified doesn't exist, we return the deliberately
            // invalid empty string.
            path = "";
        }
    } else {
        path = gArgs.GetDataDirNet();
        // If a wallets directory exists, use that, otherwise default to GetDataDir
        if (fs::is_directory(path / "wallets")) {
            path /= "wallets";
        }
    }

    return path;
}

std::string GenerateMultipathDescriptorString(const std::string& key, OutputType addr_type)
{

    // Build descriptor string
    std::string desc_prefix;
    std::string desc_suffix = "/*)";
    switch (addr_type) {
    case OutputType::LEGACY: {
        desc_prefix = "pkh(" + key + "/44h";
        break;
    }
    case OutputType::P2SH_SEGWIT: {
        desc_prefix = "sh(wpkh(" + key + "/49h";
        desc_suffix += ")";
        break;
    }
    case OutputType::BECH32: {
        desc_prefix = "wpkh(" + key + "/84h";
        break;
    }
    case OutputType::BECH32M: {
        desc_prefix = "tr(" + key + "/86h";
        break;
    }
    case OutputType::UNKNOWN: {
        // We should never have a DescriptorScriptPubKeyMan for an UNKNOWN OutputType,
        // so if we get to this point something is wrong
        assert(false);
    }
    } // no default case, so the compiler can warn about missing cases
    assert(!desc_prefix.empty());

    // Mainnet derives at 0', testnet and regtest derive at 1'
    if (Params().IsTestChain()) {
        desc_prefix += "/1h";
    } else {
        desc_prefix += "/0h";
    }

    return desc_prefix + "/0h/<0;1>" + desc_suffix;
}

} // namespace wallet
