// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>

#include <base58.h>
#include <bech32.h>
#include <script/interpreter.h>
#include <script/solver.h>
#include <tinyformat.h>
#include <util/overflow.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/vector.h>

#include <algorithm>
#include <cassert>
#include <cstring>

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
        data.reserve(1 + CeilDiv(program.size() * 8, 5u));
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, program.begin(), program.end());
        return bech32::Encode(bech32::Encoding::BECH32M, m_params.Bech32HRP(), data);
    }

    std::string operator()(const CNoDestination& no) const { return {}; }
    std::string operator()(const PubKeyDestination& pk) const { return {}; }
};

/** Translate a Bech32 decoding error code into a user-facing message. */
std::string Bech32ErrorMessage(bech32::Error error)
{
    switch (error) {
    case bech32::Error::NONE: return "";
    case bech32::Error::TOO_LONG: return "Bech32 string too long";
    case bech32::Error::INVALID_CHARS_OR_MIXED_CASE: return "Invalid character or mixed case";
    case bech32::Error::MISSING_SEPARATOR: return "Missing separator";
    case bech32::Error::INVALID_SEPARATOR_POSITION: return "Invalid separator position";
    case bech32::Error::INVALID_BASE32_CHAR: return "Invalid Base 32 character";
    case bech32::Error::INVALID_CHECKSUM: return "Invalid checksum";
    case bech32::Error::INVALID_BECH32_CHECKSUM: return "Invalid Bech32 checksum";
    case bech32::Error::INVALID_BECH32M_CHECKSUM: return "Invalid Bech32m checksum";
    }
    assert(false);
}

CTxDestination DecodeBase58Destination(const std::vector<unsigned char>& data, const CChainParams& params, std::string& error_str)
{
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
        error_str = "Invalid length for Base58 address (P2PKH or P2SH)";
    } else {
        auto prefixes{Cat(params.Base58PrefixText(CChainParams::SCRIPT_ADDRESS),
                          params.Base58PrefixText(CChainParams::PUBKEY_ADDRESS))};
        const std::string last{std::move(prefixes.back())};
        prefixes.pop_back();
        error_str = strprintf("Invalid Base58 address. Expected prefix %s or %s", util::Join(prefixes, ", "), last);
    }
    return CNoDestination();
}

CTxDestination DecodeBech32Destination(const bech32::DecodeResult& dec, const CChainParams& params, std::string& error_str)
{
    std::vector<unsigned char> data;

    if (dec.data.empty()) {
        error_str = "Empty Bech32 data section";
        return CNoDestination();
    }
    // Bech32 decoding
    if (dec.hrp != params.Bech32HRP()) {
        error_str = strprintf("Invalid or unsupported prefix for Segwit (Bech32) address (expected %s, got %s)", params.Bech32HRP(), dec.hrp);
        return CNoDestination();
    }
    int version = dec.data[0]; // The first 5 bit symbol is the witness version (0-16)
    if (version == 0 && dec.encoding != bech32::Encoding::BECH32) {
        error_str = "Version 0 witness address must use Bech32 checksum";
        return CNoDestination();
    }
    if (version != 0 && dec.encoding != bech32::Encoding::BECH32M) {
        error_str = "Version 1+ witness address must use Bech32m checksum";
        return CNoDestination();
    }
    // The rest of the symbols are converted witness program bytes.
    data.reserve(((dec.data.size() - 1) * 5) / 8);
    if (ConvertBits<5, 8, false>([&](unsigned char c) { data.push_back(c); }, dec.data.begin() + 1, dec.data.end())) {
        std::string_view byte_str{data.size() == 1 ? "byte" : "bytes"};

        if (version == 0) {
            {
                WitnessV0KeyHash keyid;
                if (data.size() == keyid.size()) {
                    std::copy(data.begin(), data.end(), keyid.begin());
                    return keyid;
                }
            }
            {
                WitnessV0ScriptHash scriptid;
                if (data.size() == scriptid.size()) {
                    std::copy(data.begin(), data.end(), scriptid.begin());
                    return scriptid;
                }
            }

            error_str = strprintf("Invalid SegWit v0 address program size (%d %s), per BIP141", data.size(), byte_str);
            return CNoDestination();
        }

        if (version == 1 && data.size() == WITNESS_V1_TAPROOT_SIZE) {
            static_assert(WITNESS_V1_TAPROOT_SIZE == WitnessV1Taproot::size());
            WitnessV1Taproot tap;
            std::copy(data.begin(), data.end(), tap.begin());
            return tap;
        }

        if (CScript::IsPayToAnchor(version, data)) {
            return PayToAnchor();
        }

        if (version > 16) {
            error_str = "Invalid Bech32 address witness version";
            return CNoDestination();
        }

        if (data.size() < 2 || data.size() > BECH32_WITNESS_PROG_MAX_LEN) {
            error_str = strprintf("Invalid Bech32 address program size (%d %s)", data.size(), byte_str);
            return CNoDestination();
        }

        return WitnessUnknown{version, data};
    } else {
        error_str = strprintf("Invalid padding in Bech32 data section");
        return CNoDestination();
    }
}

CTxDestination DecodeDestination(const std::string& str, const CChainParams& params, std::string& error_str, std::vector<int>* error_locations)
{
    error_str = "";

    // Try Bech32(m) first: checking the prefix before decoding would misroute valid
    // Bech32 addresses for other networks to the Base58 decoder, producing the
    // misleading "Invalid or unsupported Segwit (Bech32) or Base58 encoding." error.
    // Decode a lowercased copy so that mixed-case input (which is invalid) can be
    // diagnosed without decoding twice; all-lowercase and all-uppercase are both valid.
    const std::string lower_str{ToLower(str)};
    const auto dec = bech32::Decode(lower_str);
    if (dec.encoding == bech32::Encoding::BECH32 || dec.encoding == bech32::Encoding::BECH32M) {
        const bool has_upper{std::ranges::any_of(str, [](char c) { return c >= 'A' && c <= 'Z'; })};
        const bool has_lower{std::ranges::any_of(str, [](char c) { return c >= 'a' && c <= 'z'; })};
        if (has_upper && has_lower) {
            error_str = "Bech32 address is mixed case";
            if (error_locations) *error_locations = bech32::LocateErrors(str).locations;
            return CNoDestination();
        }
        return DecodeBech32Destination(dec, params, error_str);
    }

    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data, 21)) {
        return DecodeBase58Destination(data, params, error_str);
    }

    // Neither Bech32 nor Base58Check decoding succeeded. Unless the input already rules
    // out Bech32 (invalid character or mixed case), report the specific Bech32 error and
    // its locations. Otherwise, the Base58 checksum or length must be wrong if the string
    // still raw-decodes as Base58; if not, the format is ambiguous. Strings beyond the
    // 90-character BIP173 limit cannot be Bech32 addresses, so one that still raw-decodes
    // as Base58 (e.g. an extended key) is diagnosed as Base58 even though LocateErrors
    // reports it as too long.
    auto [error, locations] = bech32::LocateErrors(str);
    const bool is_base58{DecodeBase58(str, data, 100)};
    const bool maybe_bech32{error != bech32::Error::INVALID_CHARS_OR_MIXED_CASE &&
                            !(error == bech32::Error::TOO_LONG && is_base58)};
    if (maybe_bech32) {
        error_str = "Bech32 address decoded with error: " + Bech32ErrorMessage(error);
        if (error_locations) *error_locations = std::move(locations);
    } else if (is_base58) {
        error_str = "Invalid checksum or length of Base58 address (P2PKH or P2SH)";
    } else {
        error_str = "Address is not valid Base58 or Bech32";
        if (error_locations) *error_locations = std::move(locations);
    }
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
    if (!data.empty()) {
        memory_cleanse(data.data(), data.size());
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
