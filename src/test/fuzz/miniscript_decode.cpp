// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <hash.h>
#include <key.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/strencodings.h>

#include <optional>

using miniscript::operator""_mst;


struct Converter {
    typedef CPubKey Key;

    std::optional<std::string> ToString(const Key& key) const {
        return HexStr(key);
    }
    const std::vector<unsigned char> ToPKBytes(const Key& key) const {
        return {key.begin(), key.end()};
    }
    const std::vector<unsigned char> ToPKHBytes(const Key& key) const {
        const auto h = Hash160(key);
        return {h.begin(), h.end()};
    }

    template<typename I>
    std::optional<Key> FromString(I first, I last) const {
        const auto bytes = ParseHex(std::string(first, last));
        Key key{bytes.begin(), bytes.end()};
        if (key.IsValid()) return key;
        return {};
    }
    template<typename I>
    std::optional<Key> FromPKBytes(I first, I last) const {
        Key key{first, last};
        if (key.IsValid()) return key;
        return {};
    }
    template<typename I>
    std::optional<Key> FromPKHBytes(I first, I last) const {
        return {};
    }
};

const Converter CONVERTER;

FUZZ_TARGET(miniscript_decode)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CScript> script = ConsumeDeserializable<CScript>(fuzzed_data_provider);
    if (!script) return;

    const auto ms = miniscript::FromScript(*script, CONVERTER);
    if (!ms) return;

    // We can roundtrip it to its string representation.
    std::string ms_str = *ms->ToString(CONVERTER);
    assert(*miniscript::FromString(ms_str, CONVERTER) == *ms);
    // The Script representation must roundtrip since we parsed it this way the first time.
    const CScript ms_script = ms->ToScript(CONVERTER);
    assert(ms_script == *script);
}
