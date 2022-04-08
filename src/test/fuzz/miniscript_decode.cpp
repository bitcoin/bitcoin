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

    bool ToString(const Key& key, std::string& ret) const {
        ret = HexStr(key);
        return true;
    }
    const std::vector<unsigned char> ToPKBytes(const Key& key) const {
        return {key.begin(), key.end()};
    }
    const std::vector<unsigned char> ToPKHBytes(const Key& key) const {
        const auto h = Hash160(key);
        return {h.begin(), h.end()};
    }

    template<typename I>
    bool FromString(I first, I last, Key& key) const {
        const auto bytes = ParseHex(std::string(first, last));
        key.Set(bytes.begin(), bytes.end());
        return key.IsValid();
    }
    template<typename I>
    bool FromPKBytes(I first, I last, CPubKey& key) const {
        key.Set(first, last);
        return key.IsValid();
    }
    template<typename I>
    bool FromPKHBytes(I first, I last, CPubKey& key) const {
        assert(last - first == 20);
        return false;
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
    std::string ms_str;
    assert(ms->ToString(CONVERTER, ms_str));
    assert(*miniscript::FromString(ms_str, CONVERTER) == *ms);
    // The Script representation must roundtrip since we parsed it this way the first time.
    const CScript ms_script = ms->ToScript(CONVERTER);
    assert(ms_script == *script);
}
