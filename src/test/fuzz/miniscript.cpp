// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <hash.h>
#include <key.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/strencodings.h>

#include <algorithm>

namespace {

using Fragment = miniscript::Fragment;
using NodeRef = miniscript::NodeRef<CPubKey>;
using Node = miniscript::Node<CPubKey>;
using Type = miniscript::Type;
using MsCtx = miniscript::MiniscriptContext;
using miniscript::operator""_mst;

//! Some pre-computed data for more efficient string roundtrips and to simulate challenges.
struct TestData {
    typedef CPubKey Key;

    // Precomputed public keys, and a dummy signature for each of them.
    std::vector<Key> dummy_keys;
    std::map<Key, int> dummy_key_idx_map;
    std::map<CKeyID, Key> dummy_keys_map;
    std::map<Key, std::pair<std::vector<unsigned char>, bool>> dummy_sigs;
    std::map<XOnlyPubKey, std::pair<std::vector<unsigned char>, bool>> schnorr_sigs;

    // Precomputed hashes of each kind.
    std::vector<std::vector<unsigned char>> sha256;
    std::vector<std::vector<unsigned char>> ripemd160;
    std::vector<std::vector<unsigned char>> hash256;
    std::vector<std::vector<unsigned char>> hash160;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> sha256_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> ripemd160_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> hash256_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> hash160_preimages;

    //! Set the precomputed data.
    void Init() {
        unsigned char keydata[32] = {1};
        // All our signatures sign (and are required to sign) this constant message.
        constexpr uint256 MESSAGE_HASH{"0000000000000000f5cd94e18b6fe77dd7aca9e35c2b0c9cbd86356c80a71065"};
        // We don't pass additional randomness when creating a schnorr signature.
        const auto EMPTY_AUX{uint256::ZERO};

        for (size_t i = 0; i < 256; i++) {
            keydata[31] = i;
            CKey privkey;
            privkey.Set(keydata, keydata + 32, true);
            const Key pubkey = privkey.GetPubKey();

            dummy_keys.push_back(pubkey);
            dummy_key_idx_map.emplace(pubkey, i);
            dummy_keys_map.insert({pubkey.GetID(), pubkey});
            XOnlyPubKey xonly_pubkey{pubkey};
            dummy_key_idx_map.emplace(xonly_pubkey, i);
            uint160 xonly_hash{Hash160(xonly_pubkey)};
            dummy_keys_map.emplace(xonly_hash, pubkey);

            std::vector<unsigned char> sig, schnorr_sig(64);
            privkey.Sign(MESSAGE_HASH, sig);
            sig.push_back(1); // SIGHASH_ALL
            dummy_sigs.insert({pubkey, {sig, i & 1}});
            assert(privkey.SignSchnorr(MESSAGE_HASH, schnorr_sig, nullptr, EMPTY_AUX));
            schnorr_sig.push_back(1); // Maximally-sized signature has sighash byte
            schnorr_sigs.emplace(XOnlyPubKey{pubkey}, std::make_pair(std::move(schnorr_sig), i & 1));

            std::vector<unsigned char> hash;
            hash.resize(32);
            CSHA256().Write(keydata, 32).Finalize(hash.data());
            sha256.push_back(hash);
            if (i & 1) sha256_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
            CHash256().Write(keydata).Finalize(hash);
            hash256.push_back(hash);
            if (i & 1) hash256_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
            hash.resize(20);
            CRIPEMD160().Write(keydata, 32).Finalize(hash.data());
            assert(hash.size() == 20);
            ripemd160.push_back(hash);
            if (i & 1) ripemd160_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
            CHash160().Write(keydata).Finalize(hash);
            hash160.push_back(hash);
            if (i & 1) hash160_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
        }
    }

    //! Get the (Schnorr or ECDSA, depending on context) signature for this pubkey.
    const std::pair<std::vector<unsigned char>, bool>* GetSig(const MsCtx script_ctx, const Key& key) const {
        if (!miniscript::IsTapscript(script_ctx)) {
            const auto it = dummy_sigs.find(key);
            if (it == dummy_sigs.end()) return nullptr;
            return &it->second;
        } else {
            const auto it = schnorr_sigs.find(XOnlyPubKey{key});
            if (it == schnorr_sigs.end()) return nullptr;
            return &it->second;
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

    const MsCtx script_ctx;

    constexpr ParserContext(MsCtx ctx) noexcept : script_ctx(ctx) {}

    bool KeyCompare(const Key& a, const Key& b) const {
        return a < b;
    }

    std::optional<std::string> ToString(const Key& key) const
    {
        auto it = TEST_DATA.dummy_key_idx_map.find(key);
        if (it == TEST_DATA.dummy_key_idx_map.end()) return {};
        uint8_t idx = it->second;
        return HexStr(std::span{&idx, 1});
    }

    std::vector<unsigned char> ToPKBytes(const Key& key) const {
        if (!miniscript::IsTapscript(script_ctx)) {
            return {key.begin(), key.end()};
        }
        const XOnlyPubKey xonly_pubkey{key};
        return {xonly_pubkey.begin(), xonly_pubkey.end()};
    }

    std::vector<unsigned char> ToPKHBytes(const Key& key) const {
        if (!miniscript::IsTapscript(script_ctx)) {
            const auto h = Hash160(key);
            return {h.begin(), h.end()};
        }
        const auto h = Hash160(XOnlyPubKey{key});
        return {h.begin(), h.end()};
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
        if (!miniscript::IsTapscript(script_ctx)) {
            Key key{first, last};
            if (key.IsValid()) return key;
            return {};
        }
        if (last - first != 32) return {};
        XOnlyPubKey xonly_pubkey;
        std::copy(first, last, xonly_pubkey.begin());
        return xonly_pubkey.GetEvenCorrespondingCPubKey();
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

    MsCtx MsContext() const {
        return script_ctx;
    }
};

//! Context that implements naive conversion from/to script only, for roundtrip testing.
struct ScriptParserContext {
    const MsCtx script_ctx;

    constexpr ScriptParserContext(MsCtx ctx) noexcept : script_ctx(ctx) {}

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

    std::vector<unsigned char> ToPKHBytes(const Key& key) const
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

    MsCtx MsContext() const {
        return script_ctx;
    }
};

//! Context to produce a satisfaction for a Miniscript node using the pre-computed data.
struct SatisfierContext : ParserContext {

    constexpr SatisfierContext(MsCtx ctx) noexcept : ParserContext(ctx) {}

    // Timelock challenges satisfaction. Make the value (deterministically) vary to explore different
    // paths.
    bool CheckAfter(uint32_t value) const { return value % 2; }
    bool CheckOlder(uint32_t value) const { return value % 2; }

    // Signature challenges fulfilled with a dummy signature, if it was one of our dummy keys.
    miniscript::Availability Sign(const CPubKey& key, std::vector<unsigned char>& sig) const {
        bool sig_available{false};
        if (auto res = TEST_DATA.GetSig(script_ctx, key)) {
            std::tie(sig, sig_available) = *res;
        }
        return sig_available ? miniscript::Availability::YES : miniscript::Availability::NO;
    }

    //! Lookup generalization for all the hash satisfactions below
    miniscript::Availability LookupHash(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage,
                                        const std::map<std::vector<unsigned char>, std::vector<unsigned char>>& map) const
    {
        const auto it = map.find(hash);
        if (it == map.end()) return miniscript::Availability::NO;
        preimage = it->second;
        return miniscript::Availability::YES;
    }
    miniscript::Availability SatSHA256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, TEST_DATA.sha256_preimages);
    }
    miniscript::Availability SatRIPEMD160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, TEST_DATA.ripemd160_preimages);
    }
    miniscript::Availability SatHASH256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, TEST_DATA.hash256_preimages);
    }
    miniscript::Availability SatHASH160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, TEST_DATA.hash160_preimages);
    }
};

//! Context to check a satisfaction against the pre-computed data.
const struct CheckerContext: BaseSignatureChecker {
    // Signature checker methods. Checks the right dummy signature is used.
    bool CheckECDSASignature(const std::vector<unsigned char>& sig, const std::vector<unsigned char>& vchPubKey,
                             const CScript& scriptCode, SigVersion sigversion) const override
    {
        const CPubKey key{vchPubKey};
        const auto it = TEST_DATA.dummy_sigs.find(key);
        if (it == TEST_DATA.dummy_sigs.end()) return false;
        return it->second.first == sig;
    }
    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion,
                               ScriptExecutionData&, ScriptError*) const override {
        XOnlyPubKey pk{pubkey};
        auto it = TEST_DATA.schnorr_sigs.find(pk);
        if (it == TEST_DATA.schnorr_sigs.end()) return false;
        return std::ranges::equal(it->second.first, sig);
    }
    bool CheckLockTime(const CScriptNum& nLockTime) const override { return nLockTime.GetInt64() & 1; }
    bool CheckSequence(const CScriptNum& nSequence) const override { return nSequence.GetInt64() & 1; }
} CHECKER_CTX;

//! Context to check for duplicates when instancing a Node.
const struct KeyComparator {
    bool KeyCompare(const CPubKey& a, const CPubKey& b) const {
        return a < b;
    }
} KEY_COMP;

// A dummy scriptsig to pass to VerifyScript (we always use Segwit v0).
const CScript DUMMY_SCRIPTSIG;

//! Construct a miniscript node as a shared_ptr.
template<typename... Args> NodeRef MakeNodeRef(Args&&... args) {
    return miniscript::MakeNodeRef<CPubKey>(miniscript::internal::NoDupCheck{}, std::forward<Args>(args)...);
}

/** Information about a yet to be constructed Miniscript node. */
struct NodeInfo {
    //! The type of this node
    Fragment fragment;
    //! The timelock value for older() and after(), the threshold value for multi() and thresh()
    uint32_t k;
    //! Keys for this node, if it has some
    std::vector<CPubKey> keys;
    //! The hash value for this node, if it has one
    std::vector<unsigned char> hash;
    //! The type requirements for the children of this node.
    std::vector<Type> subtypes;

    NodeInfo(Fragment frag): fragment(frag), k(0) {}
    NodeInfo(Fragment frag, CPubKey key): fragment(frag), k(0), keys({key}) {}
    NodeInfo(Fragment frag, uint32_t _k): fragment(frag), k(_k) {}
    NodeInfo(Fragment frag, std::vector<unsigned char> h): fragment(frag), k(0), hash(std::move(h)) {}
    NodeInfo(std::vector<Type> subt, Fragment frag): fragment(frag), k(0), subtypes(std::move(subt)) {}
    NodeInfo(std::vector<Type> subt, Fragment frag, uint32_t _k): fragment(frag), k(_k), subtypes(std::move(subt))  {}
    NodeInfo(Fragment frag, uint32_t _k, std::vector<CPubKey> _keys): fragment(frag), k(_k), keys(std::move(_keys)) {}
};

/** Pick an index in a collection from a single byte in the fuzzer's output. */
template<typename T, typename A>
T ConsumeIndex(FuzzedDataProvider& provider, A& col) {
    const uint8_t i = provider.ConsumeIntegral<uint8_t>();
    return col[i];
}

CPubKey ConsumePubKey(FuzzedDataProvider& provider) {
    return ConsumeIndex<CPubKey>(provider, TEST_DATA.dummy_keys);
}

std::vector<unsigned char> ConsumeSha256(FuzzedDataProvider& provider) {
    return ConsumeIndex<std::vector<unsigned char>>(provider, TEST_DATA.sha256);
}

std::vector<unsigned char> ConsumeHash256(FuzzedDataProvider& provider) {
    return ConsumeIndex<std::vector<unsigned char>>(provider, TEST_DATA.hash256);
}

std::vector<unsigned char> ConsumeRipemd160(FuzzedDataProvider& provider) {
    return ConsumeIndex<std::vector<unsigned char>>(provider, TEST_DATA.ripemd160);
}

std::vector<unsigned char> ConsumeHash160(FuzzedDataProvider& provider) {
    return ConsumeIndex<std::vector<unsigned char>>(provider, TEST_DATA.hash160);
}

std::optional<uint32_t> ConsumeTimeLock(FuzzedDataProvider& provider) {
    const uint32_t k = provider.ConsumeIntegral<uint32_t>();
    if (k == 0 || k >= 0x80000000) return {};
    return k;
}

/**
 * Consume a Miniscript node from the fuzzer's output.
 *
 * This version is intended to have a fixed, stable, encoding for Miniscript nodes:
 *  - The first byte sets the type of the fragment. 0, 1 and all non-leaf fragments but thresh() are a
 *    single byte.
 *  - For the other leaf fragments, the following bytes depend on their type.
 *    - For older() and after(), the next 4 bytes define the timelock value.
 *    - For pk_k(), pk_h(), and all hashes, the next byte defines the index of the value in the test data.
 *    - For multi(), the next 2 bytes define respectively the threshold and the number of keys. Then as many
 *      bytes as the number of keys define the index of each key in the test data.
 *    - For multi_a(), same as for multi() but the threshold and the keys count are encoded on two bytes.
 *    - For thresh(), the next byte defines the threshold value and the following one the number of subs.
 */
std::optional<NodeInfo> ConsumeNodeStable(MsCtx script_ctx, FuzzedDataProvider& provider, Type type_needed) {
    bool allow_B = (type_needed == ""_mst) || (type_needed << "B"_mst);
    bool allow_K = (type_needed == ""_mst) || (type_needed << "K"_mst);
    bool allow_V = (type_needed == ""_mst) || (type_needed << "V"_mst);
    bool allow_W = (type_needed == ""_mst) || (type_needed << "W"_mst);

    switch (provider.ConsumeIntegral<uint8_t>()) {
        case 0:
            if (!allow_B) return {};
            return {{Fragment::JUST_0}};
        case 1:
            if (!allow_B) return {};
            return {{Fragment::JUST_1}};
        case 2:
            if (!allow_K) return {};
            return {{Fragment::PK_K, ConsumePubKey(provider)}};
        case 3:
            if (!allow_K) return {};
            return {{Fragment::PK_H, ConsumePubKey(provider)}};
        case 4: {
            if (!allow_B) return {};
            const auto k = ConsumeTimeLock(provider);
            if (!k) return {};
            return {{Fragment::OLDER, *k}};
        }
        case 5: {
            if (!allow_B) return {};
            const auto k = ConsumeTimeLock(provider);
            if (!k) return {};
            return {{Fragment::AFTER, *k}};
        }
        case 6:
            if (!allow_B) return {};
            return {{Fragment::SHA256, ConsumeSha256(provider)}};
        case 7:
            if (!allow_B) return {};
            return {{Fragment::HASH256, ConsumeHash256(provider)}};
        case 8:
            if (!allow_B) return {};
            return {{Fragment::RIPEMD160, ConsumeRipemd160(provider)}};
        case 9:
            if (!allow_B) return {};
            return {{Fragment::HASH160, ConsumeHash160(provider)}};
        case 10: {
            if (!allow_B || IsTapscript(script_ctx)) return {};
            const auto k = provider.ConsumeIntegral<uint8_t>();
            const auto n_keys = provider.ConsumeIntegral<uint8_t>();
            if (n_keys > 20 || k == 0 || k > n_keys) return {};
            std::vector<CPubKey> keys{n_keys};
            for (auto& key: keys) key = ConsumePubKey(provider);
            return {{Fragment::MULTI, k, std::move(keys)}};
        }
        case 11:
            if (!(allow_B || allow_K || allow_V)) return {};
            return {{{"B"_mst, type_needed, type_needed}, Fragment::ANDOR}};
        case 12:
            if (!(allow_B || allow_K || allow_V)) return {};
            return {{{"V"_mst, type_needed}, Fragment::AND_V}};
        case 13:
            if (!allow_B) return {};
            return {{{"B"_mst, "W"_mst}, Fragment::AND_B}};
        case 15:
            if (!allow_B) return {};
            return {{{"B"_mst, "W"_mst}, Fragment::OR_B}};
        case 16:
            if (!allow_V) return {};
            return {{{"B"_mst, "V"_mst}, Fragment::OR_C}};
        case 17:
            if (!allow_B) return {};
            return {{{"B"_mst, "B"_mst}, Fragment::OR_D}};
        case 18:
            if (!(allow_B || allow_K || allow_V)) return {};
            return {{{type_needed, type_needed}, Fragment::OR_I}};
        case 19: {
            if (!allow_B) return {};
            auto k = provider.ConsumeIntegral<uint8_t>();
            auto n_subs = provider.ConsumeIntegral<uint8_t>();
            if (k == 0 || k > n_subs) return {};
            std::vector<Type> subtypes;
            subtypes.reserve(n_subs);
            subtypes.emplace_back("B"_mst);
            for (size_t i = 1; i < n_subs; ++i) subtypes.emplace_back("W"_mst);
            return {{std::move(subtypes), Fragment::THRESH, k}};
        }
        case 20:
            if (!allow_W) return {};
            return {{{"B"_mst}, Fragment::WRAP_A}};
        case 21:
            if (!allow_W) return {};
            return {{{"B"_mst}, Fragment::WRAP_S}};
        case 22:
            if (!allow_B) return {};
            return {{{"K"_mst}, Fragment::WRAP_C}};
        case 23:
            if (!allow_B) return {};
            return {{{"V"_mst}, Fragment::WRAP_D}};
        case 24:
            if (!allow_V) return {};
            return {{{"B"_mst}, Fragment::WRAP_V}};
        case 25:
            if (!allow_B) return {};
            return {{{"B"_mst}, Fragment::WRAP_J}};
        case 26:
            if (!allow_B) return {};
            return {{{"B"_mst}, Fragment::WRAP_N}};
        case 27: {
            if (!allow_B || !IsTapscript(script_ctx)) return {};
            const auto k = provider.ConsumeIntegral<uint16_t>();
            const auto n_keys = provider.ConsumeIntegral<uint16_t>();
            if (n_keys > 999 || k == 0 || k > n_keys) return {};
            std::vector<CPubKey> keys{n_keys};
            for (auto& key: keys) key = ConsumePubKey(provider);
            return {{Fragment::MULTI_A, k, std::move(keys)}};
        }
        default:
            break;
    }
    return {};
}

/* This structure contains a table which for each "target" Type a list of recipes
 * to construct it, automatically inferred from the behavior of ComputeType.
 * Note that the Types here are not the final types of the constructed Nodes, but
 * just the subset that are required. For example, a recipe for the "Bo" type
 * might construct a "Bondu" sha256() NodeInfo, but cannot construct a "Bz" older().
 * Each recipe is a Fragment together with a list of required types for its subnodes.
 */
struct SmartInfo
{
    using recipe = std::pair<Fragment, std::vector<Type>>;
    std::map<Type, std::vector<recipe>> wsh_table, tap_table;

    void Init()
    {
        Init(wsh_table, MsCtx::P2WSH);
        Init(tap_table, MsCtx::TAPSCRIPT);
    }

    void Init(std::map<Type, std::vector<recipe>>& table, MsCtx script_ctx)
    {
        /* Construct a set of interesting type requirements to reason with (sections of BKVWzondu). */
        std::vector<Type> types;
        for (int base = 0; base < 4; ++base) { /* select from B,K,V,W */
            Type type_base = base == 0 ? "B"_mst : base == 1 ? "K"_mst : base == 2 ? "V"_mst : "W"_mst;
            for (int zo = 0; zo < 3; ++zo) { /* select from z,o,(none) */
                Type type_zo = zo == 0 ? "z"_mst : zo == 1 ? "o"_mst : ""_mst;
                for (int n = 0; n < 2; ++n) { /* select from (none),n */
                    if (zo == 0 && n == 1) continue; /* z conflicts with n */
                    if (base == 3 && n == 1) continue; /* W conflicts with n */
                    Type type_n = n == 0 ? ""_mst : "n"_mst;
                    for (int d = 0; d < 2; ++d) { /* select from (none),d */
                        if (base == 2 && d == 1) continue; /* V conflicts with d */
                        Type type_d = d == 0 ? ""_mst : "d"_mst;
                        for (int u = 0; u < 2; ++u) { /* select from (none),u */
                            if (base == 2 && u == 1) continue; /* V conflicts with u */
                            Type type_u = u == 0 ? ""_mst : "u"_mst;
                            Type type = type_base | type_zo | type_n | type_d | type_u;
                            types.push_back(type);
                        }
                    }
                }
            }
        }

        /* We define a recipe a to be a super-recipe of recipe b if they use the same
         * fragment, the same number of subexpressions, and each of a's subexpression
         * types is a supertype of the corresponding subexpression type of b.
         * Within the set of recipes for the construction of a given type requirement,
         * no recipe should be a super-recipe of another (as the super-recipe is
         * applicable in every place the sub-recipe is, the sub-recipe is redundant). */
        auto is_super_of = [](const recipe& a, const recipe& b) {
            if (a.first != b.first) return false;
            if (a.second.size() != b.second.size()) return false;
            for (size_t i = 0; i < a.second.size(); ++i) {
                if (!(b.second[i] << a.second[i])) return false;
            }
            return true;
        };

        /* Sort the type requirements. Subtypes will always sort later (e.g. Bondu will
         * sort after Bo or Bu). As we'll be constructing recipes using these types, in
         * order, in what follows, we'll construct super-recipes before sub-recipes.
         * That means we never need to go back and delete a sub-recipe because a
         * super-recipe got added. */
        std::sort(types.begin(), types.end());

        // Iterate over all possible fragments.
        for (int fragidx = 0; fragidx <= int(Fragment::MULTI_A); ++fragidx) {
            int sub_count = 0; //!< The minimum number of child nodes this recipe has.
            int sub_range = 1; //!< The maximum number of child nodes for this recipe is sub_count+sub_range-1.
            size_t data_size = 0;
            size_t n_keys = 0;
            uint32_t k = 0;
            Fragment frag{fragidx};

            // Only produce recipes valid in the given context.
            if ((!miniscript::IsTapscript(script_ctx) && frag == Fragment::MULTI_A)
                || (miniscript::IsTapscript(script_ctx) && frag == Fragment::MULTI)) {
                continue;
            }

            // Based on the fragment, determine #subs/data/k/keys to pass to ComputeType. */
            switch (frag) {
                case Fragment::PK_K:
                case Fragment::PK_H:
                    n_keys = 1;
                    break;
                case Fragment::MULTI:
                case Fragment::MULTI_A:
                    n_keys = 1;
                    k = 1;
                    break;
                case Fragment::OLDER:
                case Fragment::AFTER:
                    k = 1;
                    break;
                case Fragment::SHA256:
                case Fragment::HASH256:
                    data_size = 32;
                    break;
                case Fragment::RIPEMD160:
                case Fragment::HASH160:
                    data_size = 20;
                    break;
                case Fragment::JUST_0:
                case Fragment::JUST_1:
                    break;
                case Fragment::WRAP_A:
                case Fragment::WRAP_S:
                case Fragment::WRAP_C:
                case Fragment::WRAP_D:
                case Fragment::WRAP_V:
                case Fragment::WRAP_J:
                case Fragment::WRAP_N:
                    sub_count = 1;
                    break;
                case Fragment::AND_V:
                case Fragment::AND_B:
                case Fragment::OR_B:
                case Fragment::OR_C:
                case Fragment::OR_D:
                case Fragment::OR_I:
                    sub_count = 2;
                    break;
                case Fragment::ANDOR:
                    sub_count = 3;
                    break;
                case Fragment::THRESH:
                    // Thresh logic is executed for 1 and 2 arguments. Larger numbers use ad-hoc code to extend.
                    sub_count = 1;
                    sub_range = 2;
                    k = 1;
                    break;
            }

            // Iterate over the number of subnodes (sub_count...sub_count+sub_range-1).
            std::vector<Type> subt;
            for (int subs = sub_count; subs < sub_count + sub_range; ++subs) {
                // Iterate over the possible subnode types (at most 3).
                for (Type x : types) {
                    for (Type y : types) {
                        for (Type z : types) {
                            // Compute the resulting type of a node with the selected fragment / subnode types.
                            subt.clear();
                            if (subs > 0) subt.push_back(x);
                            if (subs > 1) subt.push_back(y);
                            if (subs > 2) subt.push_back(z);
                            Type res = miniscript::internal::ComputeType(frag, x, y, z, subt, k, data_size, subs, n_keys, script_ctx);
                            // Continue if the result is not a valid node.
                            if ((res << "K"_mst) + (res << "V"_mst) + (res << "B"_mst) + (res << "W"_mst) != 1) continue;

                            recipe entry{frag, subt};
                            auto super_of_entry = [&](const recipe& rec) { return is_super_of(rec, entry); };
                            // Iterate over all supertypes of res (because if e.g. our selected fragment/subnodes result
                            // in a Bondu, they can form a recipe that is also applicable for constructing a B, Bou, Bdu, ...).
                            for (Type s : types) {
                                if ((res & "BKVWzondu"_mst) << s) {
                                    auto& recipes = table[s];
                                    // If we don't already have a super-recipe to the new one, add it.
                                    if (!std::any_of(recipes.begin(), recipes.end(), super_of_entry)) {
                                        recipes.push_back(entry);
                                    }
                                }
                            }

                            if (subs <= 2) break;
                        }
                        if (subs <= 1) break;
                    }
                    if (subs <= 0) break;
                }
            }
        }

        /* Find which types are useful. The fuzzer logic only cares about constructing
         * B,V,K,W nodes, so any type that isn't needed in any recipe (directly or
         * indirectly) for the construction of those is uninteresting. */
        std::set<Type> useful_types{"B"_mst, "V"_mst, "K"_mst, "W"_mst};
        // Find the transitive closure by adding types until the set of types does not change.
        while (true) {
            size_t set_size = useful_types.size();
            for (const auto& [type, recipes] : table) {
                if (useful_types.count(type) != 0) {
                    for (const auto& [_, subtypes] : recipes) {
                        for (auto subtype : subtypes) useful_types.insert(subtype);
                    }
                }
            }
            if (useful_types.size() == set_size) break;
        }
        // Remove all rules that construct uninteresting types.
        for (auto type_it = table.begin(); type_it != table.end();) {
            if (useful_types.count(type_it->first) == 0) {
                type_it = table.erase(type_it);
            } else {
                ++type_it;
            }
        }

        /* Find which types are constructible. A type is constructible if there is a leaf
         * node recipe for constructing it, or a recipe whose subnodes are all constructible.
         * Types can be non-constructible because they have no recipes to begin with,
         * because they can only be constructed using recipes that involve otherwise
         * non-constructible types, or because they require infinite recursion. */
        std::set<Type> constructible_types{};
        auto known_constructible = [&](Type type) { return constructible_types.count(type) != 0; };
        // Find the transitive closure by adding types until the set of types does not change.
        while (true) {
            size_t set_size = constructible_types.size();
            // Iterate over all types we have recipes for.
            for (const auto& [type, recipes] : table) {
                if (!known_constructible(type)) {
                    // For not (yet known to be) constructible types, iterate over their recipes.
                    for (const auto& [_, subt] : recipes) {
                        // If any recipe involves only (already known to be) constructible types,
                        // add the recipe's type to the set.
                        if (std::all_of(subt.begin(), subt.end(), known_constructible)) {
                            constructible_types.insert(type);
                            break;
                        }
                    }
                }
            }
            if (constructible_types.size() == set_size) break;
        }
        for (auto type_it = table.begin(); type_it != table.end();) {
            // Remove all recipes which involve non-constructible types.
            type_it->second.erase(std::remove_if(type_it->second.begin(), type_it->second.end(),
                [&](const recipe& rec) {
                    return !std::all_of(rec.second.begin(), rec.second.end(), known_constructible);
                }), type_it->second.end());
            // Delete types entirely which have no recipes left.
            if (type_it->second.empty()) {
                type_it = table.erase(type_it);
            } else {
                ++type_it;
            }
        }

        for (auto& [type, recipes] : table) {
            // Sort recipes for determinism, and place those using fewer subnodes first.
            // This avoids runaway expansion (when reaching the end of the fuzz input,
            // all zeroes are read, resulting in the first available recipe being picked).
            std::sort(recipes.begin(), recipes.end(),
                [](const recipe& a, const recipe& b) {
                    if (a.second.size() < b.second.size()) return true;
                    if (a.second.size() > b.second.size()) return false;
                    return a < b;
                }
            );
        }
    }
} SMARTINFO;

/**
 * Consume a Miniscript node from the fuzzer's output.
 *
 * This is similar to ConsumeNodeStable, but uses a precomputed table with permitted
 * fragments/subnode type for each required type. It is intended to more quickly explore
 * interesting miniscripts, at the cost of higher implementation complexity (which could
 * cause it miss things if incorrect), and with less regard for stability of the seeds
 * (as improvements to the tables or changes to the typing rules could invalidate
 * everything).
 */
std::optional<NodeInfo> ConsumeNodeSmart(MsCtx script_ctx, FuzzedDataProvider& provider, Type type_needed) {
    /** Table entry for the requested type. */
    const auto& table{IsTapscript(script_ctx) ? SMARTINFO.tap_table : SMARTINFO.wsh_table};
    auto recipes_it = table.find(type_needed);
    assert(recipes_it != table.end());
    /** Pick one recipe from the available ones for that type. */
    const auto& [frag, subt] = PickValue(provider, recipes_it->second);

    // Based on the fragment the recipe uses, fill in other data (k, keys, data).
    switch (frag) {
        case Fragment::PK_K:
        case Fragment::PK_H:
            return {{frag, ConsumePubKey(provider)}};
        case Fragment::MULTI: {
            const auto n_keys = provider.ConsumeIntegralInRange<uint8_t>(1, 20);
            const auto k = provider.ConsumeIntegralInRange<uint8_t>(1, n_keys);
            std::vector<CPubKey> keys{n_keys};
            for (auto& key: keys) key = ConsumePubKey(provider);
            return {{frag, k, std::move(keys)}};
        }
        case Fragment::MULTI_A: {
            const auto n_keys = provider.ConsumeIntegralInRange<uint16_t>(1, 999);
            const auto k = provider.ConsumeIntegralInRange<uint16_t>(1, n_keys);
            std::vector<CPubKey> keys{n_keys};
            for (auto& key: keys) key = ConsumePubKey(provider);
            return {{frag, k, std::move(keys)}};
        }
        case Fragment::OLDER:
        case Fragment::AFTER:
            return {{frag, provider.ConsumeIntegralInRange<uint32_t>(1, 0x7FFFFFF)}};
        case Fragment::SHA256:
            return {{frag, PickValue(provider, TEST_DATA.sha256)}};
        case Fragment::HASH256:
            return {{frag, PickValue(provider, TEST_DATA.hash256)}};
        case Fragment::RIPEMD160:
            return {{frag, PickValue(provider, TEST_DATA.ripemd160)}};
        case Fragment::HASH160:
            return {{frag, PickValue(provider, TEST_DATA.hash160)}};
        case Fragment::JUST_0:
        case Fragment::JUST_1:
        case Fragment::WRAP_A:
        case Fragment::WRAP_S:
        case Fragment::WRAP_C:
        case Fragment::WRAP_D:
        case Fragment::WRAP_V:
        case Fragment::WRAP_J:
        case Fragment::WRAP_N:
        case Fragment::AND_V:
        case Fragment::AND_B:
        case Fragment::OR_B:
        case Fragment::OR_C:
        case Fragment::OR_D:
        case Fragment::OR_I:
        case Fragment::ANDOR:
            return {{subt, frag}};
        case Fragment::THRESH: {
            uint32_t children;
            if (subt.size() < 2) {
                children = subt.size();
            } else {
                // If we hit a thresh with 2 subnodes, artificially extend it to any number
                // (2 or larger) by replicating the type of the last subnode.
                children = provider.ConsumeIntegralInRange<uint32_t>(2, MAX_OPS_PER_SCRIPT / 2);
            }
            auto k = provider.ConsumeIntegralInRange<uint32_t>(1, children);
            std::vector<Type> subs = subt;
            while (subs.size() < children) subs.push_back(subs.back());
            return {{std::move(subs), frag, k}};
        }
    }

    assert(false);
}

/**
 * Generate a Miniscript node based on the fuzzer's input.
 *
 * - ConsumeNode is a function object taking a Type, and returning an std::optional<NodeInfo>.
 * - root_type is the required type properties of the constructed NodeRef.
 * - strict_valid sets whether ConsumeNode is expected to guarantee a NodeInfo that results in
 *   a NodeRef whose Type() matches the type fed to ConsumeNode.
 */
template<typename F>
NodeRef GenNode(MsCtx script_ctx, F ConsumeNode, Type root_type, bool strict_valid = false) {
    /** A stack of miniscript Nodes being built up. */
    std::vector<NodeRef> stack;
    /** The queue of instructions. */
    std::vector<std::pair<Type, std::optional<NodeInfo>>> todo{{root_type, {}}};
    /** Predict the number of (static) script ops. */
    uint32_t ops{0};
    /** Predict the total script size (every unexplored subnode is counted as one, as every leaf is
     *  at least one script byte). */
    uint32_t scriptsize{1};

    while (!todo.empty()) {
        // The expected type we have to construct.
        auto type_needed = todo.back().first;
        if (!todo.back().second) {
            // Fragment/children have not been decided yet. Decide them.
            auto node_info = ConsumeNode(type_needed);
            if (!node_info) return {};
            // Update predicted resource limits. Since every leaf Miniscript node is at least one
            // byte long, we move one byte from each child to their parent. A similar technique is
            // used in the miniscript::internal::Parse function to prevent runaway string parsing.
            scriptsize += miniscript::internal::ComputeScriptLen(node_info->fragment, ""_mst, node_info->subtypes.size(), node_info->k, node_info->subtypes.size(),
                                                                 node_info->keys.size(), script_ctx) - 1;
            if (scriptsize > MAX_STANDARD_P2WSH_SCRIPT_SIZE) return {};
            switch (node_info->fragment) {
            case Fragment::JUST_0:
            case Fragment::JUST_1:
                break;
            case Fragment::PK_K:
                break;
            case Fragment::PK_H:
                ops += 3;
                break;
            case Fragment::OLDER:
            case Fragment::AFTER:
                ops += 1;
                break;
            case Fragment::RIPEMD160:
            case Fragment::SHA256:
            case Fragment::HASH160:
            case Fragment::HASH256:
                ops += 4;
                break;
            case Fragment::ANDOR:
                ops += 3;
                break;
            case Fragment::AND_V:
                break;
            case Fragment::AND_B:
            case Fragment::OR_B:
                ops += 1;
                break;
            case Fragment::OR_C:
                ops += 2;
                break;
            case Fragment::OR_D:
                ops += 3;
                break;
            case Fragment::OR_I:
                ops += 3;
                break;
            case Fragment::THRESH:
                ops += node_info->subtypes.size();
                break;
            case Fragment::MULTI:
                ops += 1;
                break;
            case Fragment::MULTI_A:
                ops += node_info->keys.size() + 1;
                break;
            case Fragment::WRAP_A:
                ops += 2;
                break;
            case Fragment::WRAP_S:
                ops += 1;
                break;
            case Fragment::WRAP_C:
                ops += 1;
                break;
            case Fragment::WRAP_D:
                ops += 3;
                break;
            case Fragment::WRAP_V:
                // We don't account for OP_VERIFY here; that will be corrected for when the actual
                // node is constructed below.
                break;
            case Fragment::WRAP_J:
                ops += 4;
                break;
            case Fragment::WRAP_N:
                ops += 1;
                break;
            }
            if (ops > MAX_OPS_PER_SCRIPT) return {};
            auto subtypes = node_info->subtypes;
            todo.back().second = std::move(node_info);
            todo.reserve(todo.size() + subtypes.size());
            // As elements on the todo stack are processed back to front, construct
            // them in reverse order (so that the first subnode is generated first).
            for (size_t i = 0; i < subtypes.size(); ++i) {
                todo.emplace_back(*(subtypes.rbegin() + i), std::nullopt);
            }
        } else {
            // The back of todo has fragment and number of children decided, and
            // those children have been constructed at the back of stack. Pop
            // that entry off todo, and use it to construct a new NodeRef on
            // stack.
            NodeInfo& info = *todo.back().second;
            // Gather children from the back of stack.
            std::vector<NodeRef> sub;
            sub.reserve(info.subtypes.size());
            for (size_t i = 0; i < info.subtypes.size(); ++i) {
                sub.push_back(std::move(*(stack.end() - info.subtypes.size() + i)));
            }
            stack.erase(stack.end() - info.subtypes.size(), stack.end());
            // Construct new NodeRef.
            NodeRef node;
            if (info.keys.empty()) {
                node = MakeNodeRef(script_ctx, info.fragment, std::move(sub), std::move(info.hash), info.k);
            } else {
                assert(sub.empty());
                assert(info.hash.empty());
                node = MakeNodeRef(script_ctx, info.fragment, std::move(info.keys), info.k);
            }
            // Verify acceptability.
            if (!node || (node->GetType() & "KVWB"_mst) == ""_mst) {
                assert(!strict_valid);
                return {};
            }
            if (!(type_needed == ""_mst)) {
                assert(node->GetType() << type_needed);
            }
            if (!node->IsValid()) return {};
            // Update resource predictions.
            if (node->fragment == Fragment::WRAP_V && node->subs[0]->GetType() << "x"_mst) {
                ops += 1;
                scriptsize += 1;
            }
            if (!miniscript::IsTapscript(script_ctx) && ops > MAX_OPS_PER_SCRIPT) return {};
            if (scriptsize > miniscript::internal::MaxScriptSize(script_ctx)) {
                return {};
            }
            // Move it to the stack.
            stack.push_back(std::move(node));
            todo.pop_back();
        }
    }
    assert(stack.size() == 1);
    assert(stack[0]->GetStaticOps() == ops);
    assert(stack[0]->ScriptSize() == scriptsize);
    stack[0]->DuplicateKeyCheck(KEY_COMP);
    return std::move(stack[0]);
}

//! The spk for this script under the given context. If it's a Taproot output also record the spend data.
CScript ScriptPubKey(MsCtx ctx, const CScript& script, TaprootBuilder& builder)
{
    if (!miniscript::IsTapscript(ctx)) return CScript() << OP_0 << WitnessV0ScriptHash(script);

    // For Taproot outputs we always use a tree with a single script and a dummy internal key.
    builder.Add(0, script, TAPROOT_LEAF_TAPSCRIPT);
    builder.Finalize(XOnlyPubKey::NUMS_H);
    return GetScriptForDestination(builder.GetOutput());
}

//! Fill the witness with the data additional to the script satisfaction.
void SatisfactionToWitness(MsCtx ctx, CScriptWitness& witness, const CScript& script, TaprootBuilder& builder) {
    // For P2WSH, it's only the witness script.
    witness.stack.emplace_back(script.begin(), script.end());
    if (!miniscript::IsTapscript(ctx)) return;
    // For Tapscript we also need the control block.
    witness.stack.push_back(*builder.GetSpendData().scripts.begin()->second.begin());
}

/** Perform various applicable tests on a miniscript Node. */
void TestNode(const MsCtx script_ctx, const NodeRef& node, FuzzedDataProvider& provider)
{
    if (!node) return;

    // Check that it roundtrips to text representation
    const ParserContext parser_ctx{script_ctx};
    std::optional<std::string> str{node->ToString(parser_ctx)};
    assert(str);
    auto parsed = miniscript::FromString(*str, parser_ctx);
    assert(parsed);
    assert(*parsed == *node);

    // Check consistency between script size estimation and real size.
    auto script = node->ToScript(parser_ctx);
    assert(node->ScriptSize() == script.size());

    // Check consistency of "x" property with the script (type K is excluded, because it can end
    // with a push of a key, which could match these opcodes).
    if (!(node->GetType() << "K"_mst)) {
        bool ends_in_verify = !(node->GetType() << "x"_mst);
        assert(ends_in_verify == (script.back() == OP_CHECKSIG || script.back() == OP_CHECKMULTISIG || script.back() == OP_EQUAL || script.back() == OP_NUMEQUAL));
    }

    // The rest of the checks only apply when testing a valid top-level script.
    if (!node->IsValidTopLevel()) return;

    // Check roundtrip to script
    auto decoded = miniscript::FromScript(script, parser_ctx);
    assert(decoded);
    // Note we can't use *decoded == *node because the miniscript representation may differ, so we check that:
    // - The script corresponding to that decoded form matches exactly
    // - The type matches exactly
    assert(decoded->ToScript(parser_ctx) == script);
    assert(decoded->GetType() == node->GetType());

    // Optionally pad the script or the witness in order to increase the sensitivity of the tests of
    // the resources limits logic.
    CScriptWitness witness_mal, witness_nonmal;
    if (provider.ConsumeBool()) {
        // Under P2WSH, optionally pad the script with OP_NOPs to max op the ops limit of the constructed script.
        // This makes the script obviously not actually miniscript-compatible anymore, but the
        // signatures constructed in this test don't commit to the script anyway, so the same
        // miniscript satisfier will work. This increases the sensitivity of the test to the ops
        // counting logic being too low, especially for simple scripts.
        // Do this optionally because we're not solely interested in cases where the number of ops is
        // maximal.
        // Do not pad more than what would cause MAX_STANDARD_P2WSH_SCRIPT_SIZE to be reached, however,
        // as that also invalidates scripts.
        const auto node_ops{node->GetOps()};
        if (!IsTapscript(script_ctx) && node_ops && *node_ops < MAX_OPS_PER_SCRIPT
            && node->ScriptSize() < MAX_STANDARD_P2WSH_SCRIPT_SIZE) {
            int add = std::min<int>(
                MAX_OPS_PER_SCRIPT - *node_ops,
                MAX_STANDARD_P2WSH_SCRIPT_SIZE - node->ScriptSize());
            for (int i = 0; i < add; ++i) script.push_back(OP_NOP);
        }

        // Under Tapscript, optionally pad the stack up to the limit minus the calculated maximum execution stack
        // size to assert a Miniscript would never add more elements to the stack during execution than anticipated.
        const auto node_exec_ss{node->GetExecStackSize()};
        if (miniscript::IsTapscript(script_ctx) && node_exec_ss && *node_exec_ss < MAX_STACK_SIZE) {
            unsigned add{(unsigned)MAX_STACK_SIZE - *node_exec_ss};
            witness_mal.stack.resize(add);
            witness_nonmal.stack.resize(add);
            script.reserve(add);
            for (unsigned i = 0; i < add; ++i) script.push_back(OP_NIP);
        }
    }

    const SatisfierContext satisfier_ctx{script_ctx};

    // Get the ScriptPubKey for this script, filling spend data if it's Taproot.
    TaprootBuilder builder;
    const CScript script_pubkey{ScriptPubKey(script_ctx, script, builder)};

    // Run malleable satisfaction algorithm.
    std::vector<std::vector<unsigned char>> stack_mal;
    const bool mal_success = node->Satisfy(satisfier_ctx, stack_mal, false) == miniscript::Availability::YES;

    // Run non-malleable satisfaction algorithm.
    std::vector<std::vector<unsigned char>> stack_nonmal;
    const bool nonmal_success = node->Satisfy(satisfier_ctx, stack_nonmal, true) == miniscript::Availability::YES;

    if (nonmal_success) {
        // Non-malleable satisfactions are bounded by the satisfaction size plus:
        // - For P2WSH spends, the witness script
        // - For Tapscript spends, both the witness script and the control block
        const size_t max_stack_size{*node->GetStackSize() + 1 + miniscript::IsTapscript(script_ctx)};
        assert(stack_nonmal.size() <= max_stack_size);
        // If a non-malleable satisfaction exists, the malleable one must also exist, and be identical to it.
        assert(mal_success);
        assert(stack_nonmal == stack_mal);
        // Compute witness size (excluding script push, control block, and witness count encoding).
        const uint64_t wit_size{GetSerializeSize(stack_nonmal) - GetSizeOfCompactSize(stack_nonmal.size())};
        assert(wit_size <= *node->GetWitnessSize());

        // Test non-malleable satisfaction.
        witness_nonmal.stack.insert(witness_nonmal.stack.end(), std::make_move_iterator(stack_nonmal.begin()), std::make_move_iterator(stack_nonmal.end()));
        SatisfactionToWitness(script_ctx, witness_nonmal, script, builder);
        ScriptError serror;
        bool res = VerifyScript(DUMMY_SCRIPTSIG, script_pubkey, &witness_nonmal, STANDARD_SCRIPT_VERIFY_FLAGS, CHECKER_CTX, &serror);
        // Non-malleable satisfactions are guaranteed to be valid if ValidSatisfactions().
        if (node->ValidSatisfactions()) assert(res);
        // More detailed: non-malleable satisfactions must be valid, or could fail with ops count error (if CheckOpsLimit failed),
        // or with a stack size error (if CheckStackSize check failed).
        assert(res ||
               (!node->CheckOpsLimit() && serror == ScriptError::SCRIPT_ERR_OP_COUNT) ||
               (!node->CheckStackSize() && serror == ScriptError::SCRIPT_ERR_STACK_SIZE));
    }

    if (mal_success && (!nonmal_success || witness_mal.stack != witness_nonmal.stack)) {
        // Test malleable satisfaction only if it's different from the non-malleable one.
        witness_mal.stack.insert(witness_mal.stack.end(), std::make_move_iterator(stack_mal.begin()), std::make_move_iterator(stack_mal.end()));
        SatisfactionToWitness(script_ctx, witness_mal, script, builder);
        ScriptError serror;
        bool res = VerifyScript(DUMMY_SCRIPTSIG, script_pubkey, &witness_mal, STANDARD_SCRIPT_VERIFY_FLAGS, CHECKER_CTX, &serror);
        // Malleable satisfactions are not guaranteed to be valid under any conditions, but they can only
        // fail due to stack or ops limits.
        assert(res || serror == ScriptError::SCRIPT_ERR_OP_COUNT || serror == ScriptError::SCRIPT_ERR_STACK_SIZE);
    }

    if (node->IsSane()) {
        // For sane nodes, the two algorithms behave identically.
        assert(mal_success == nonmal_success);
    }

    // Verify that if a node is policy-satisfiable, the malleable satisfaction
    // algorithm succeeds. Given that under IsSane() both satisfactions
    // are identical, this implies that for such nodes, the non-malleable
    // satisfaction will also match the expected policy.
    const auto is_key_satisfiable = [script_ctx](const CPubKey& pubkey) -> bool {
        auto sig_ptr{TEST_DATA.GetSig(script_ctx, pubkey)};
        return sig_ptr != nullptr && sig_ptr->second;
    };
    bool satisfiable = node->IsSatisfiable([&](const Node& node) -> bool {
        switch (node.fragment) {
        case Fragment::PK_K:
        case Fragment::PK_H:
            return is_key_satisfiable(node.keys[0]);
        case Fragment::MULTI:
        case Fragment::MULTI_A: {
            size_t sats = std::count_if(node.keys.begin(), node.keys.end(), [&](const auto& key) {
                return size_t(is_key_satisfiable(key));
            });
            return sats >= node.k;
        }
        case Fragment::OLDER:
        case Fragment::AFTER:
            return node.k & 1;
        case Fragment::SHA256:
            return TEST_DATA.sha256_preimages.count(node.data);
        case Fragment::HASH256:
            return TEST_DATA.hash256_preimages.count(node.data);
        case Fragment::RIPEMD160:
            return TEST_DATA.ripemd160_preimages.count(node.data);
        case Fragment::HASH160:
            return TEST_DATA.hash160_preimages.count(node.data);
        default:
            assert(false);
        }
        return false;
    });
    assert(mal_success == satisfiable);
}

} // namespace

void FuzzInit()
{
    static ECC_Context ecc_context{};
    TEST_DATA.Init();
}

void FuzzInitSmart()
{
    FuzzInit();
    SMARTINFO.Init();
}

/** Fuzz target that runs TestNode on nodes generated using ConsumeNodeStable. */
FUZZ_TARGET(miniscript_stable, .init = FuzzInit)
{
    // Run it under both P2WSH and Tapscript contexts.
    for (const auto script_ctx: {MsCtx::P2WSH, MsCtx::TAPSCRIPT}) {
        FuzzedDataProvider provider(buffer.data(), buffer.size());
        TestNode(script_ctx, GenNode(script_ctx, [&](Type needed_type) {
            return ConsumeNodeStable(script_ctx, provider, needed_type);
        }, ""_mst), provider);
    }
}

/** Fuzz target that runs TestNode on nodes generated using ConsumeNodeSmart. */
FUZZ_TARGET(miniscript_smart, .init = FuzzInitSmart)
{
    /** The set of types we aim to construct nodes for. Together they cover all. */
    static constexpr std::array<Type, 4> BASE_TYPES{"B"_mst, "V"_mst, "K"_mst, "W"_mst};

    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const auto script_ctx{(MsCtx)provider.ConsumeBool()};
    TestNode(script_ctx, GenNode(script_ctx, [&](Type needed_type) {
        return ConsumeNodeSmart(script_ctx, provider, needed_type);
    }, PickValue(provider, BASE_TYPES), true), provider);
}

/* Fuzz tests that test parsing from a string, and roundtripping via string. */
FUZZ_TARGET(miniscript_string, .init = FuzzInit)
{
    if (buffer.empty()) return;
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto str = provider.ConsumeBytesAsString(provider.remaining_bytes() - 1);
    const ParserContext parser_ctx{(MsCtx)provider.ConsumeBool()};
    auto parsed = miniscript::FromString(str, parser_ctx);
    if (!parsed) return;

    const auto str2 = parsed->ToString(parser_ctx);
    assert(str2);
    auto parsed2 = miniscript::FromString(*str2, parser_ctx);
    assert(parsed2);
    assert(*parsed == *parsed2);
}

/* Fuzz tests that test parsing from a script, and roundtripping via script. */
FUZZ_TARGET(miniscript_script)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CScript> script = ConsumeDeserializable<CScript>(fuzzed_data_provider);
    if (!script) return;

    const ScriptParserContext script_parser_ctx{(MsCtx)fuzzed_data_provider.ConsumeBool()};
    const auto ms = miniscript::FromScript(*script, script_parser_ctx);
    if (!ms) return;

    assert(ms->ToScript(script_parser_ctx) == *script);
}
