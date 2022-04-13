// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <hash.h>
#include <key.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/strencodings.h>

namespace {

//! Context that implements naive conversion from/to script only, for roundtrip testing.
struct ScriptParserContext {
    //! For Script roundtrip we never need the key from a key hash.
    struct Key {
        bool is_hash;
        std::vector<unsigned char> data;
    };

    const std::vector<unsigned char>& ToPKBytes(const Key& key) const
    {
        assert(!key.is_hash);
        return key.data;
    }

    const std::vector<unsigned char> ToPKHBytes(const Key& key) const
    {
        if (key.is_hash) return key.data;
        const auto h = Hash160(key.data);
        return {h.begin(), h.end()};
    }

    template<typename I>
    std::optional<Key> FromPKBytes(I first, I last) const
    {
        Key key;
        key.data.assign(first, last);
        key.is_hash = false;
        return key;
    }

    template<typename I>
    std::optional<Key> FromPKHBytes(I first, I last) const
    {
        Key key;
        key.data.assign(first, last);
        key.is_hash = true;
        return key;
    }
} SCRIPT_PARSER_CONTEXT;

}

/* Fuzz tests that test parsing from a script, and roundtripping via script. */
FUZZ_TARGET(miniscript_script)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CScript> script = ConsumeDeserializable<CScript>(fuzzed_data_provider);
    if (!script) return;

    const auto ms = miniscript::FromScript(*script, SCRIPT_PARSER_CONTEXT);
    if (!ms) return;

    assert(ms->ToScript(SCRIPT_PARSER_CONTEXT) == *script);
}
