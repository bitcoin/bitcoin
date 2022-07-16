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

//! Some pre-computed data for more efficient string roundtrips.
struct TestData {
    typedef CPubKey Key;

    // Precomputed public keys.
    std::vector<Key> dummy_keys;
    std::map<Key, int> dummy_key_idx_map;
    std::map<CKeyID, Key> dummy_keys_map;

    //! Set the precomputed data.
    void Init() {
        unsigned char keydata[32] = {1};
        for (size_t i = 0; i < 256; i++) {
            keydata[31] = i;
            CKey privkey;
            privkey.Set(keydata, keydata + 32, true);
            const Key pubkey = privkey.GetPubKey();

            dummy_keys.push_back(pubkey);
            dummy_key_idx_map.emplace(pubkey, i);
            dummy_keys_map.insert({pubkey.GetID(), pubkey});
        }
    }
} TEST_DATA;

/**
 * Context to parse a Miniscript node to and from Script or text representation.
 * Uses an integer (an index in the dummy keys array from the test data) as keys in order
 * to focus on fuzzing the Miniscript nodes' test representation, not the key representation.
 */
struct ParserContext {
    typedef CPubKey Key;

    bool KeyCompare(const Key& a, const Key& b) const {
        return a < b;
    }

    std::optional<std::string> ToString(const Key& key) const
    {
        auto it = TEST_DATA.dummy_key_idx_map.find(key);
        if (it == TEST_DATA.dummy_key_idx_map.end()) return {};
        uint8_t idx = it->second;
        return HexStr(Span{&idx, 1});
    }

    template<typename I>
    std::optional<Key> FromString(I first, I last) const {
        if (last - first != 2) return {};
        auto idx = ParseHex(std::string(first, last));
        if (idx.size() != 1) return {};
        return TEST_DATA.dummy_keys[idx[0]];
    }

    template<typename I>
    std::optional<Key> FromPKBytes(I first, I last) const {
        Key key;
        key.Set(first, last);
        if (!key.IsValid()) return {};
        return key;
    }

    template<typename I>
    std::optional<Key> FromPKHBytes(I first, I last) const {
        assert(last - first == 20);
        CKeyID keyid;
        std::copy(first, last, keyid.begin());
        const auto it = TEST_DATA.dummy_keys_map.find(keyid);
        if (it == TEST_DATA.dummy_keys_map.end()) return {};
        return it->second;
    }
} PARSER_CTX;

//! Context that implements naive conversion from/to script only, for roundtrip testing.
struct ScriptParserContext {
    //! For Script roundtrip we never need the key from a key hash.
    struct Key {
        bool is_hash;
        std::vector<unsigned char> data;
    };

    bool KeyCompare(const Key& a, const Key& b) const {
        return a.data < b.data;
    }

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

} // namespace

void FuzzInit()
{
    ECC_Start();
    TEST_DATA.Init();
}

/* Fuzz tests that test parsing from a string, and roundtripping via string. */
FUZZ_TARGET_INIT(miniscript_string, FuzzInit)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto str = provider.ConsumeRemainingBytesAsString();
    auto parsed = miniscript::FromString(str, PARSER_CTX);
    if (!parsed) return;

    const auto str2 = parsed->ToString(PARSER_CTX);
    assert(str2);
    auto parsed2 = miniscript::FromString(*str2, PARSER_CTX);
    assert(parsed2);
    assert(*parsed == *parsed2);
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
