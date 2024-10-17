// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>

#include <base58.h>
#include <bech32.h>
#include <script/interpreter.h>
#include <script/solver.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <algorithm>
#include <assert.h>
#include <string.h>

/// Maximum witness length for Bech32 addresses.
static constexpr std::size_t BECH32_WITNESS_PROG_MAX_LEN = 40;

namespace {
class DestinationEncoder
{
private:
    const CChainParams& m_params;

public:
    explicit DestinationEncoder(const CChainParams& params) : m_params(params) {}

    std::string operator()(const PKHash& id) const
    {
        std::vector<unsigned char> data = m_params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
        data.insert(data.end(), id.begin(), id.end());
        return EncodeBase58Check(data);
    }

    std::string operator()(const ScriptHash& id) const
    {
        std::vector<unsigned char> data = m_params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
        data.insert(data.end(), id.begin(), id.end());
        return EncodeBase58Check(data);
    }

    std::string operator()(const WitnessV0KeyHash& id) const
    {
        std::vector<unsigned char> data = {0};
        data.reserve(33);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
        return bech32::Encode(bech32::Encoding::BECH32, m_params.Bech32HRP(), data);
    }

    std::string operator()(const WitnessV0ScriptHash& id) const
    {
        std::vector<unsigned char> data = {0};
        data.reserve(53);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
        return bech32::Encode(bech32::Encoding::BECH32, m_params.Bech32HRP(), data);
    }

    std::string operator()(const WitnessV1Taproot& tap) const
    {
        std::vector<unsigned char> data = {1};
        data.reserve(53);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, tap.begin(), tap.end());
        return bech32::Encode(bech32::Encoding::BECH32M, m_params.Bech32HRP(), data);
    }

    std::string operator()(const WitnessUnknown& id) const
    {
        const std::vector<unsigned char>& program = id.GetWitnessProgram();
        if (id.GetWitnessVersion() < 1 || id.GetWitnessVersion() > 16 || program.size() < 2 || program.size() > 40) {
            return {};
        }
        std::vector<unsigned char> data = {(unsigned char)id.GetWitnessVersion()};
        data.reserve(1 + (program.size() * 8 + 4) / 5);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, program.begin(), program.end());
        return bech32::Encode(bech32::Encoding::BECH32M, m_params.Bech32HRP(), data);
    }

    std::string operator()(const CNoDestination& no) const { return {}; }
    std::string operator()(const PubKeyDestination& pk) const { return {}; }
};

CTxDestination DecodeDestination(const std::string& str, const CChainParams& params, std::string& error_str, std::vector<int>* error_locations)
{
    error_str = "";

    static const uint8_t MAX_BASE58_CHARS = 100;
    static const uint8_t MAX_BASE58_CHECK_CHARS = 21;

    std::vector<unsigned char> data, bech32_data;

    auto [bech32_encoding, bech32_hrp, bech32_chars] = bech32::Decode(str);
    auto [bech32_error, bech32_error_loc] = bech32::LocateErrors(str);

    bool is_bech32 = bech32_encoding != bech32::Encoding::INVALID;
    bool is_base58_check = DecodeBase58Check(str, data, MAX_BASE58_CHECK_CHARS);

    if (!is_bech32 && is_base58_check) {
        uint160 hash;
        // base58-encoded Bitcoin addresses.
        // Public-key-hash-addresses have version 0 (or 111 testnet).
        // The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
        const std::vector<unsigned char>& pubkey_prefix = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
        if (data.size() == hash.size() + pubkey_prefix.size() && std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin())) {
            std::copy(data.begin() + pubkey_prefix.size(), data.end(), hash.begin());
            return PKHash(hash);
        }
        // Script-hash-addresses have version 5 (or 196 testnet).
        // The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
        const std::vector<unsigned char>& script_prefix = params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
        if (data.size() == hash.size() + script_prefix.size() && std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) {
            std::copy(data.begin() + script_prefix.size(), data.end(), hash.begin());
            return ScriptHash(hash);
        }

        // If the prefix of data matches either the script or pubkey prefix, the length must have been wrong
        if ((data.size() >= script_prefix.size() &&
                std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) ||
            (data.size() >= pubkey_prefix.size() &&
                std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin()))) {
            error_str = "Invalid length for Base58 address";
        } else {
            const std::vector<std::string_view>& pubkey_prefixes = params.Base58EncodedPrefix(CChainParams::PUBKEY_ADDRESS);
            const std::vector<std::string_view>& script_prefixes = params.Base58EncodedPrefix(CChainParams::SCRIPT_ADDRESS);

            std::vector<std::string_view> encoded_prefixes;
            encoded_prefixes.insert(encoded_prefixes.end(), script_prefixes.begin(), script_prefixes.end());
            encoded_prefixes.insert(encoded_prefixes.end(), pubkey_prefixes.begin(), pubkey_prefixes.end());

            std::string base58_address_prefixes;
            for (size_t i = 0; i < encoded_prefixes.size(); ++i) {
                if (i > 0) {
                    base58_address_prefixes += (i == encoded_prefixes.size() - 1) ? ", or " : ", ";
                }
                base58_address_prefixes += std::string(encoded_prefixes[i]);  // Convert string_view to string
            }

            error_str = strprintf("Invalid Base58 %s address. Expected prefix %s", params.GetChainTypeDisplayString(), base58_address_prefixes);
        }
        return CNoDestination();
    } else if (!is_bech32) {
        bool is_base58 = DecodeBase58(str, data, MAX_BASE58_CHARS);
        bool is_validBech32Chars = (bech32_error != "Invalid Base 32 character" &&
                                    bech32_error != "Invalid character or mixed case");
        // Try Base58 decoding without the checksum, using a much larger max length
        if (!is_base58) {
            // If bech32 decoding failed due to invalid base32 chars, address format is ambiguous; otherwise, report bech32 error
            error_str = is_validBech32Chars ? "Bech32(m) address decoded with error: " + bech32_error : "Address is not valid Base58 or Bech32";
        } else {
            // This covers the case where an address is encoded as valid base58 and invalid bech32(m) due to a non base32 error
            error_str = is_validBech32Chars ? "Invalid Base58 or Bech32(m) address" : "Invalid checksum or length of Base58 address";
        }

        if (is_validBech32Chars && error_locations) {
            *error_locations = std::move(bech32_error_loc);
        }

        return CNoDestination();
    }

    if (bech32_encoding == bech32::Encoding::BECH32 || bech32_encoding == bech32::Encoding::BECH32M) {
        if (bech32_chars.empty()) {
            error_str = "Empty Bech32 data section";
            return CNoDestination();
        }
        // Bech32 decoding
        if (bech32_hrp != params.Bech32HRP()) {
            error_str = strprintf("Invalid or unsupported prefix for %s address (expected %s, got %s)", params.GetChainTypeDisplayString(), params.Bech32HRP(), bech32_hrp);
            return CNoDestination();
        }
        int version = bech32_chars[0]; // The first 5 bit symbol is the witness version (0-16)
        if (version == 0 && bech32_encoding != bech32::Encoding::BECH32) {
            error_str = "Version 0 witness address must use Bech32 checksum";
            return CNoDestination();
        }
        if (version != 0 && bech32_encoding != bech32::Encoding::BECH32M) {
            error_str = "Version 1+ witness address must use Bech32m checksum";
            return CNoDestination();
        }
        // The rest of the symbols are converted witness program bytes.
        bech32_data.reserve(((bech32_chars.size() - 1) * 5) / 8);
        if (ConvertBits<5, 8, false>([&](unsigned char c) { bech32_data.push_back(c); }, bech32_chars.begin() + 1, bech32_chars.end())) {
            std::string_view byte_str{bech32_data.size() == 1 ? "byte" : "bytes"};

            if (version == 0) {
                {
                    WitnessV0KeyHash keyid;
                    if (bech32_data.size() == keyid.size()) {
                        std::copy(bech32_data.begin(), bech32_data.end(), keyid.begin());
                        return keyid;
                    }
                }
                {
                    WitnessV0ScriptHash scriptid;
                    if (bech32_data.size() == scriptid.size()) {
                        std::copy(bech32_data.begin(), bech32_data.end(), scriptid.begin());
                        return scriptid;
                    }
                }

                error_str = strprintf("Invalid SegWit v0 address program size (%d %s), per BIP141", bech32_data.size(), byte_str);
                return CNoDestination();
            }

            if (version == 1 && bech32_data.size() == WITNESS_V1_TAPROOT_SIZE) {
                static_assert(WITNESS_V1_TAPROOT_SIZE == WitnessV1Taproot::size());
                WitnessV1Taproot tap;
                std::copy(bech32_data.begin(), bech32_data.end(), tap.begin());
                return tap;
            }

            if (CScript::IsPayToAnchor(version, bech32_data)) {
                return PayToAnchor();
            }

            if (version > 16) {
                error_str = "Invalid Bech32 address witness version";
                return CNoDestination();
            }

            if (bech32_data.size() < 2 || bech32_data.size() > BECH32_WITNESS_PROG_MAX_LEN) {
                error_str = strprintf("Invalid Bech32 address program size (%d %s)", bech32_data.size(), byte_str);
                return CNoDestination();
            }

            return WitnessUnknown{version, bech32_data};
        } else {
            error_str = strprintf("Invalid padding in Bech32 data section");
            return CNoDestination();
        }
    }

    // Return results of Bech32(m) error location
    error_str = bech32_error;
    if (error_locations) *error_locations = std::move(bech32_error_loc);
    return CNoDestination();
}
} // namespace

CKey DecodeSecret(const std::string& str)
{
    CKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data, 34)) {
        const std::vector<unsigned char>& privkey_prefix = Params().Base58Prefix(CChainParams::SECRET_KEY);
        if ((data.size() == 32 + privkey_prefix.size() || (data.size() == 33 + privkey_prefix.size() && data.back() == 1)) &&
            std::equal(privkey_prefix.begin(), privkey_prefix.end(), data.begin())) {
            bool compressed = data.size() == 33 + privkey_prefix.size();
            key.Set(data.begin() + privkey_prefix.size(), data.begin() + privkey_prefix.size() + 32, compressed);
        }
    }
    if (!data.empty()) {
        memory_cleanse(data.data(), data.size());
    }
    return key;
}

std::string EncodeSecret(const CKey& key)
{
    assert(key.IsValid());
    std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::SECRET_KEY);
    data.insert(data.end(), UCharCast(key.begin()), UCharCast(key.end()));
    if (key.IsCompressed()) {
        data.push_back(1);
    }
    std::string ret = EncodeBase58Check(data);
    memory_cleanse(data.data(), data.size());
    return ret;
}

CExtPubKey DecodeExtPubKey(const std::string& str)
{
    CExtPubKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data, 78)) {
        const std::vector<unsigned char>& prefix = Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
        if (data.size() == BIP32_EXTKEY_SIZE + prefix.size() && std::equal(prefix.begin(), prefix.end(), data.begin())) {
            key.Decode(data.data() + prefix.size());
        }
    }
    return key;
}

std::string EncodeExtPubKey(const CExtPubKey& key)
{
    std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
    size_t size = data.size();
    data.resize(size + BIP32_EXTKEY_SIZE);
    key.Encode(data.data() + size);
    std::string ret = EncodeBase58Check(data);
    return ret;
}

CExtKey DecodeExtKey(const std::string& str)
{
    CExtKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data, 78)) {
        const std::vector<unsigned char>& prefix = Params().Base58Prefix(CChainParams::EXT_SECRET_KEY);
        if (data.size() == BIP32_EXTKEY_SIZE + prefix.size() && std::equal(prefix.begin(), prefix.end(), data.begin())) {
            key.Decode(data.data() + prefix.size());
        }
    }
    return key;
}

std::string EncodeExtKey(const CExtKey& key)
{
    std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::EXT_SECRET_KEY);
    size_t size = data.size();
    data.resize(size + BIP32_EXTKEY_SIZE);
    key.Encode(data.data() + size);
    std::string ret = EncodeBase58Check(data);
    memory_cleanse(data.data(), data.size());
    return ret;
}

std::string EncodeDestination(const CTxDestination& dest)
{
    return std::visit(DestinationEncoder(Params()), dest);
}

CTxDestination DecodeDestination(const std::string& str, std::string& error_msg, std::vector<int>* error_locations)
{
    return DecodeDestination(str, Params(), error_msg, error_locations);
}

CTxDestination DecodeDestination(const std::string& str)
{
    std::string error_msg;
    return DecodeDestination(str, error_msg);
}

bool IsValidDestinationString(const std::string& str, const CChainParams& params)
{
    std::string error_msg;
    return IsValidDestination(DecodeDestination(str, params, error_msg, nullptr));
}

bool IsValidDestinationString(const std::string& str)
{
    return IsValidDestinationString(str, Params());
}
