// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletutil.h>

#include <chainparams.h>
#include <common/args.h>
#include <key_io.h>
#include <logging.h>

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

WalletDescriptor GenerateWalletDescriptor(const CExtPubKey& master_key, const OutputType& addr_type, bool internal)
{
    int64_t creation_time = GetTime();

    std::string xpub = EncodeExtPubKey(master_key);

    // Build descriptor string
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

    std::string internal_path = internal ? "/1" : "/0";
    std::string desc_str = desc_prefix + "/0h" + internal_path + desc_suffix;

    // Make the descriptor
    FlatSigningProvider keys;
    std::string error;
    std::vector<std::unique_ptr<Descriptor>> desc = Parse(desc_str, keys, error, false);
    WalletDescriptor w_desc(std::move(desc.at(0)), creation_time, 0, 0, 0);
    return w_desc;
}

MockableCursor::MockableCursor(const MockableData& records, bool pass, Span<const std::byte> prefix)
{
    m_pass = pass;
    std::tie(m_cursor, m_cursor_end) = records.equal_range(BytePrefix{prefix});
}

DatabaseCursor::Status MockableCursor::Next(DataStream& key, DataStream& value)
{
    if (!m_pass) {
        return Status::FAIL;
    }
    if (m_cursor == m_cursor_end) {
        return Status::DONE;
    }
    key.clear();
    value.clear();
    const auto& [key_data, value_data] = *m_cursor;
    key.write(key_data);
    value.write(value_data);
    m_cursor++;
    return Status::MORE;
}

bool MockableBatch::ReadKey(DataStream&& key, DataStream& value)
{
    if (!m_pass) {
        return false;
    }
    SerializeData key_data{key.begin(), key.end()};
    const auto& it = m_records.find(key_data);
    if (it == m_records.end()) {
        return false;
    }
    value.clear();
    value.write(it->second);
    return true;
}

bool MockableBatch::WriteKey(DataStream&& key, DataStream&& value, bool overwrite)
{
    if (!m_pass) {
        return false;
    }
    SerializeData key_data{key.begin(), key.end()};
    SerializeData value_data{value.begin(), value.end()};
    auto [it, inserted] = m_records.emplace(key_data, value_data);
    if (!inserted && overwrite) { // Overwrite if requested
        it->second = value_data;
        inserted = true;
    }
    return inserted;
}

bool MockableBatch::EraseKey(DataStream&& key)
{
    if (!m_pass) {
        return false;
    }
    SerializeData key_data{key.begin(), key.end()};
    m_records.erase(key_data);
    return true;
}

bool MockableBatch::HasKey(DataStream&& key)
{
    if (!m_pass) {
        return false;
    }
    SerializeData key_data{key.begin(), key.end()};
    return m_records.count(key_data) > 0;
}

bool MockableBatch::ErasePrefix(Span<const std::byte> prefix)
{
    if (!m_pass) {
        return false;
    }
    auto it = m_records.begin();
    while (it != m_records.end()) {
        auto& key = it->first;
        if (key.size() < prefix.size() || std::search(key.begin(), key.end(), prefix.begin(), prefix.end()) != key.begin()) {
            it++;
            continue;
        }
        it = m_records.erase(it);
    }
    return true;
}

std::unique_ptr<WalletDatabase> CreateMockableWalletDatabase(MockableData records)
{
    return std::make_unique<MockableDatabase>(records);
}

} // namespace wallet
