// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletutil.h>

#include <chainparams.h>
#include <common/args.h>
#include <key_io.h>
#include <logging.h>
#include <script/solver.h>

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

bool IsFeatureSupported(int wallet_version, int feature_version)
{
    return wallet_version >= feature_version;
}

WalletFeature GetClosestWalletFeature(int version)
{
    static constexpr std::array wallet_features{FEATURE_LATEST, FEATURE_PRE_SPLIT_KEYPOOL, FEATURE_NO_DEFAULT_KEY, FEATURE_HD_SPLIT, FEATURE_HD, FEATURE_COMPRPUBKEY, FEATURE_WALLETCRYPT, FEATURE_BASE};
    for (const WalletFeature& wf : wallet_features) {
        if (version >= wf) return wf;
    }
    return static_cast<WalletFeature>(0);
}

WalletDescriptor GenerateWalletDescriptor(const CExtKey& master_key, const OutputType& addr_type, bool internal, std::vector<CKey>& out_keys)
{
    int64_t creation_time = GetTime();

    std::string xpriv = EncodeExtKey(master_key);
    std::string xpub = EncodeExtPubKey(master_key.Neuter());

    // Build descriptor string
    std::string desc_str;
    std::string desc_prefix;
    std::string desc_suffix = "/*)";
    switch (addr_type) {
    case OutputType::LEGACY: {
        desc_prefix = "pkh(" + xpub + "/44h";
        break;
    }
    case OutputType::P2SH_SEGWIT: {
        desc_prefix = "sh(wpkh(" + xpub + "/49h";
        desc_suffix += ")";
        break;
    }
    case OutputType::BECH32: {
        desc_prefix = "wpkh(" + xpub + "/84h";
        break;
    }
    case OutputType::BECH32M: {
        desc_prefix = "tr(" + xpub + "/86h";
        break;
    }
    case OutputType::SILENT_PAYMENTS: {
        // The actual scan and spend keys will be derived from these XPRIVs
        // The actual XPRIV and the spend key will not be retained in the descriptor,
        // but the scan key will be.
        desc_str = "sp(" + xpriv + "/352h/0h/0h/1h/0," + xpriv + "/352h/0h/0h/0h/0)";
        break;
    }
    case OutputType::UNKNOWN: {
        // We should never have a DescriptorScriptPubKeyMan for an UNKNOWN OutputType,
        // so if we get to this point something is wrong
        assert(false);
    }
    } // no default case, so the compiler can warn about missing cases
    assert(!desc_str.empty() || !desc_prefix.empty());

    if (desc_str.empty()) {
        // Mainnet derives at 0', testnet and regtest derive at 1'
        if (Params().IsTestChain()) {
            desc_prefix += "/1h";
        } else {
            desc_prefix += "/0h";
        }

        std::string internal_path = internal ? "/1" : "/0";
        desc_str = desc_prefix + "/0h" + internal_path + desc_suffix;
    }

    // Make the descriptor
    FlatSigningProvider keys;
    std::string error;
    std::vector<std::unique_ptr<Descriptor>> desc = Parse(desc_str, keys, error, false);
    for (auto& key : keys.keys) {
        out_keys.push_back(std::move(key.second));
    }
    assert(addr_type != OutputType::SILENT_PAYMENTS || out_keys.size() == 1);
    WalletDescriptor w_desc(std::move(desc.at(0)), creation_time, 0, 0, 0);
    return w_desc;
}

std::optional<std::pair<std::vector<XOnlyPubKey>, bip352::PublicData>> GetSilentPaymentsData(const CTransaction& tx, const std::map<COutPoint, Coin>& spent_coins)
{
    std::vector<XOnlyPubKey> output_keys;
    for (const CTxOut& txout : tx.vout) {
        std::vector<std::vector<unsigned char>> solutions;
        TxoutType type = Solver(txout.scriptPubKey, solutions);
        if (type == TxoutType::WITNESS_V1_TAPROOT) {
            XOnlyPubKey xonlypubkey{solutions[0]};
            if (xonlypubkey.IsFullyValid()) {
                output_keys.emplace_back(std::move(xonlypubkey));
            }
        } else if (type == TxoutType::WITNESS_UNKNOWN) {
            // Cannot have outputs with unknown witness versions
            return std::nullopt;
        }
    }

    // Must have at least one taproot output
    if (output_keys.size() == 0) return std::nullopt;

    auto public_data = bip352::GetSilentPaymentsPublicData(tx.vin, spent_coins);
    if (!public_data.has_value()) return std::nullopt;

    return std::make_pair(output_keys, std::move(public_data.value()));
}

} // namespace wallet
