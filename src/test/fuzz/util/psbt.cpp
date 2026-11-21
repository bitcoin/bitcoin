// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util/psbt.h>

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <pubkey.h>
#include <random.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <algorithm>
#include <array>
#include <ios>
#include <optional>
#include <span>
#include <vector>

using namespace util::hex_literals;

namespace {

struct Record {
    //! Raw "<keytype><keydata>" bytes, excluding the length prefix.
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
};

//! A global, input or output map. All share the same record layout.
using Map = std::vector<Record>;

template <typename... Args>
std::vector<uint8_t> Ser(const Args&... args)
{
    std::vector<uint8_t> out;
    VectorWriter{out, 0, args...};
    return out;
}

/**
 * Split the buffer into maps until it runs out. A map that is cut short is
 * dropped, but the maps before it are kept. Returns no maps if the magic is
 * missing.
 */
std::vector<Map> ParsePSBT(std::span<const uint8_t> buffer)
{
    std::vector<Map> maps;
    if (buffer.size() < std::size(PSBT_MAGIC_BYTES) || !std::ranges::equal(buffer.first(std::size(PSBT_MAGIC_BYTES)), PSBT_MAGIC_BYTES)) {
        return maps;
    }
    SpanReader s{buffer.subspan(std::size(PSBT_MAGIC_BYTES))};
    try {
        while (!s.empty()) {
            Map map;
            while (true) {
                Record rec;
                s >> rec.key;
                // A zero-length key is the separator that ends the map.
                if (rec.key.empty()) break;
                s >> rec.value;
                map.push_back(std::move(rec));
            }
            maps.push_back(std::move(map));
        }
    } catch (const std::ios_base::failure&) {
    }
    return maps;
}

std::vector<uint8_t> SerializePSBT(const std::vector<Map>& maps)
{
    std::vector<uint8_t> out{std::begin(PSBT_MAGIC_BYTES), std::end(PSBT_MAGIC_BYTES)};
    VectorWriter w{out, out.size()};
    for (const Map& map : maps) {
        for (const Record& rec : map) {
            w << rec.key << rec.value;
        }
        w << PSBT_SEPARATOR;
    }
    return out;
}

//! Minimal valid PSBTv0: a global unsigned tx with 1 input and 1 output, followed by empty input/output maps.
std::vector<uint8_t> BuildPSBTv0Template()
{
    CMutableTransaction tx;
    tx.version = 2;
    tx.vin.emplace_back(COutPoint{Txid{}, 0});
    tx.vout.emplace_back(0, CScript{});

    std::vector<Map> maps(3);
    maps[0].push_back({{PSBT_GLOBAL_UNSIGNED_TX}, Ser(TX_NO_WITNESS(tx))});
    return SerializePSBT(maps);
}

//! Minimal valid PSBTv2 with 1 input and 1 output, required fields only.
std::vector<uint8_t> BuildPSBTv2Template()
{
    std::vector<Map> maps(3);
    maps[0].push_back({{PSBT_GLOBAL_VERSION}, Ser(uint32_t{2})});
    maps[0].push_back({{PSBT_GLOBAL_TX_VERSION}, Ser(uint32_t{2})});
    maps[0].push_back({{PSBT_GLOBAL_INPUT_COUNT}, Ser(COMPACTSIZE(uint64_t{1}))});
    maps[0].push_back({{PSBT_GLOBAL_OUTPUT_COUNT}, Ser(COMPACTSIZE(uint64_t{1}))});

    maps[1].push_back({{PSBT_IN_PREVIOUS_TXID}, Ser(uint256{})});
    maps[1].push_back({{PSBT_IN_OUTPUT_INDEX}, Ser(uint32_t{0})});

    maps[2].push_back({{PSBT_OUT_AMOUNT}, Ser(CAmount{0})});
    maps[2].push_back({{PSBT_OUT_SCRIPT}, {}});
    return SerializePSBT(maps);
}

//! Bitmask of the PSBT versions a key type is allowed in.
constexpr uint8_t V0{1 << 0};
constexpr uint8_t V2{1 << 1};
constexpr uint8_t ANY_VERSION{V0 | V2};

enum class KeyData {
    NONE,          //!< single-byte key
    PUBKEY,        //!< 33-byte compressed pubkey
    XONLY,         //!< 32-byte x-only pubkey
    XONLY_LEAF,    //!< x-only pubkey followed by a 32-byte leaf hash
    CONTROL_BLOCK, //!< Taproot control block: <leaf version|parity><x-only internal key><32-byte node>*
    XPUB,          //!< 78-byte BIP32 extended pubkey, including the version bytes
    MUSIG2_IDS,    //!< participant pubkey, aggregate pubkey and an optional 32-byte leaf hash
    PROPRIETARY,   //!< <compact size identifier><compact size subtype><keydata>
    BYTES20,       //!< 20 random bytes, e.g. a RIPEMD160/HASH160 hash
    BYTES32,       //!< 32 random bytes, e.g. a SHA256/HASH256 hash
};

enum class ValueKind {
    RANDOM,         //!< value_len random bytes
    TX,             //!< transaction with empty scriptSigs and witnesses
    TXOUT,          //!< <8-byte amount><compact size script len><script>
    DER_SIG,        //!< strictly DER-encoded ECDSA signature followed by SIGHASH_ALL
    KEY_ORIGIN,     //!< <4-byte fingerprint><4-byte index>*
    TAP_KEY_ORIGIN, //!< <compact size n><n leaf hashes><key origin>
    PUBKEYS,        //!< concatenated compressed pubkeys
    TAP_TREE,       //!< complete tree of <depth><leaf version><compact size script len><script> leaves
    PSBT_VERSION,   //!< PSBT version 0 or 2, the only ones Bitcoin Core accepts
};

struct KnownKey {
    uint8_t type;
    uint8_t versions;
    size_t value_len{0}; //!< 0 => pick a random length
    KeyData key_data{KeyData::NONE};
    ValueKind value_kind{ValueKind::RANDOM};
};

//! The secp256k1 generator point G and 2G, compressed. Pubkeys in keydata are validated, so random bytes would almost never pass.
constexpr auto VALID_PUBKEYS{std::to_array({
    "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"_hex_u8,
    "02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5"_hex_u8,
})};
//! The x coordinate of G.
constexpr auto VALID_XONLY{"79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"_hex_u8};
//! Mainnet xpub version bytes.
constexpr auto XPUB_VERSION{"0488b21e"_hex_u8};

// BIP 174 / BIP 370 key types, tagged with the versions that allow them.
constexpr KnownKey GLOBAL_KEYS[]{
    {PSBT_GLOBAL_UNSIGNED_TX, V0, 0, KeyData::NONE, ValueKind::TX},
    {PSBT_GLOBAL_XPUB, ANY_VERSION, 0, KeyData::XPUB, ValueKind::KEY_ORIGIN},
    {PSBT_GLOBAL_TX_VERSION, V2, 4},
    {PSBT_GLOBAL_FALLBACK_LOCKTIME, V2, 4},
    {PSBT_GLOBAL_INPUT_COUNT, V2, 1},
    {PSBT_GLOBAL_OUTPUT_COUNT, V2, 1},
    {PSBT_GLOBAL_TX_MODIFIABLE, V2, 1},
    {PSBT_GLOBAL_VERSION, ANY_VERSION, 0, KeyData::NONE, ValueKind::PSBT_VERSION},
    {PSBT_GLOBAL_PROPRIETARY, ANY_VERSION, 0, KeyData::PROPRIETARY},
};
constexpr KnownKey INPUT_KEYS[]{
    {PSBT_IN_NON_WITNESS_UTXO, ANY_VERSION, 0, KeyData::NONE, ValueKind::TX},
    {PSBT_IN_WITNESS_UTXO, ANY_VERSION, 0, KeyData::NONE, ValueKind::TXOUT},
    {PSBT_IN_PARTIAL_SIG, ANY_VERSION, 0, KeyData::PUBKEY, ValueKind::DER_SIG},
    {PSBT_IN_SIGHASH, ANY_VERSION, 4},
    {PSBT_IN_REDEEMSCRIPT, ANY_VERSION},
    {PSBT_IN_WITNESSSCRIPT, ANY_VERSION},
    {PSBT_IN_BIP32_DERIVATION, ANY_VERSION, 0, KeyData::PUBKEY, ValueKind::KEY_ORIGIN},
    {PSBT_IN_SCRIPTSIG, ANY_VERSION},
    {PSBT_IN_SCRIPTWITNESS, ANY_VERSION},
    {PSBT_IN_RIPEMD160, ANY_VERSION, 0, KeyData::BYTES20},
    {PSBT_IN_SHA256, ANY_VERSION, 0, KeyData::BYTES32},
    {PSBT_IN_HASH160, ANY_VERSION, 0, KeyData::BYTES20},
    {PSBT_IN_HASH256, ANY_VERSION, 0, KeyData::BYTES32},
    {PSBT_IN_PREVIOUS_TXID, V2, 32},
    {PSBT_IN_OUTPUT_INDEX, V2, 4},
    {PSBT_IN_SEQUENCE, V2, 4},
    {PSBT_IN_REQUIRED_TIME_LOCKTIME, V2, 4},
    {PSBT_IN_REQUIRED_HEIGHT_LOCKTIME, V2, 4},
    {PSBT_IN_TAP_KEY_SIG, ANY_VERSION, 64},
    {PSBT_IN_TAP_SCRIPT_SIG, ANY_VERSION, 64, KeyData::XONLY_LEAF},
    {PSBT_IN_TAP_LEAF_SCRIPT, ANY_VERSION, 0, KeyData::CONTROL_BLOCK},
    {PSBT_IN_TAP_BIP32_DERIVATION, ANY_VERSION, 0, KeyData::XONLY, ValueKind::TAP_KEY_ORIGIN},
    {PSBT_IN_TAP_INTERNAL_KEY, ANY_VERSION, 32},
    {PSBT_IN_TAP_MERKLE_ROOT, ANY_VERSION, 32},
    {PSBT_IN_MUSIG2_PARTICIPANT_PUBKEYS, ANY_VERSION, 0, KeyData::PUBKEY, ValueKind::PUBKEYS},
    {PSBT_IN_MUSIG2_PUB_NONCE, ANY_VERSION, MUSIG2_PUBNONCE_SIZE, KeyData::MUSIG2_IDS},
    {PSBT_IN_MUSIG2_PARTIAL_SIG, ANY_VERSION, 32, KeyData::MUSIG2_IDS},
    {PSBT_IN_PROPRIETARY, ANY_VERSION, 0, KeyData::PROPRIETARY},
};
constexpr KnownKey OUTPUT_KEYS[]{
    {PSBT_OUT_REDEEMSCRIPT, ANY_VERSION},
    {PSBT_OUT_WITNESSSCRIPT, ANY_VERSION},
    {PSBT_OUT_BIP32_DERIVATION, ANY_VERSION, 0, KeyData::PUBKEY, ValueKind::KEY_ORIGIN},
    {PSBT_OUT_AMOUNT, V2, 8},
    {PSBT_OUT_SCRIPT, V2},
    {PSBT_OUT_TAP_INTERNAL_KEY, ANY_VERSION, 32},
    {PSBT_OUT_TAP_TREE, ANY_VERSION, 0, KeyData::NONE, ValueKind::TAP_TREE},
    {PSBT_OUT_TAP_BIP32_DERIVATION, ANY_VERSION, 0, KeyData::XONLY, ValueKind::TAP_KEY_ORIGIN},
    {PSBT_OUT_MUSIG2_PARTICIPANT_PUBKEYS, ANY_VERSION, 0, KeyData::PUBKEY, ValueKind::PUBKEYS},
    {PSBT_OUT_PROPRIETARY, ANY_VERSION, 0, KeyData::PROPRIETARY},
};

const Record* FindRecord(const Map& map, uint8_t type)
{
    const auto it{std::ranges::find_if(map, [&](const Record& rec) { return std::ranges::equal(rec.key, std::array{type}); })};
    return it == map.end() ? nullptr : &*it;
}

//! Bitcoin Core parses both versions, so detect the PSBT_GLOBAL_VERSION the global map declares.
uint8_t DetectVersion(const Map& global)
{
    const Record* rec{FindRecord(global, PSBT_GLOBAL_VERSION)};
    return rec && std::ranges::equal(rec->value, Ser(uint32_t{2})) ? V2 : V0;
}

//! Number of input maps the global map declares, if it can be decoded.
std::optional<uint64_t> InputCount(const Map& global, uint8_t version)
{
    try {
        if (version == V2) {
            const Record* rec{FindRecord(global, PSBT_GLOBAL_INPUT_COUNT)};
            if (!rec) return std::nullopt;
            SpanReader s{rec->value};
            return ReadCompactSize(s);
        }
        const Record* rec{FindRecord(global, PSBT_GLOBAL_UNSIGNED_TX)};
        if (!rec) return std::nullopt;
        CMutableTransaction tx;
        SpanReader{rec->value} >> TX_NO_WITNESS(tx);
        return tx.vin.size();
    } catch (const std::ios_base::failure&) {
        return std::nullopt;
    }
}

//! Key pool for the map at map_idx: the global map comes first, then the input maps, then the output maps.
std::span<const KnownKey> PoolFor(size_t map_idx, std::optional<uint64_t> num_inputs, InsecureRandomContext& rng)
{
    if (map_idx == 0) return GLOBAL_KEYS;
    const bool is_input{num_inputs ? map_idx <= *num_inputs : rng.randbool()};
    return is_input ? std::span<const KnownKey>{INPUT_KEYS} : std::span<const KnownKey>{OUTPUT_KEYS};
}

void Append(std::vector<uint8_t>& out, std::span<const uint8_t> bytes)
{
    out.insert(out.end(), bytes.begin(), bytes.end());
}

std::span<const uint8_t> RandomPubkey(InsecureRandomContext& rng)
{
    return VALID_PUBKEYS[rng.randrange(VALID_PUBKEYS.size())];
}

std::vector<uint8_t> MakeKeyData(KeyData kind, InsecureRandomContext& rng)
{
    std::vector<uint8_t> out;
    switch (kind) {
    case KeyData::NONE:
        break;
    case KeyData::PUBKEY:
        Append(out, RandomPubkey(rng));
        break;
    case KeyData::XONLY:
        Append(out, VALID_XONLY);
        break;
    case KeyData::XONLY_LEAF:
        Append(out, VALID_XONLY);
        Append(out, rng.randbytes<uint8_t>(32));
        break;
    case KeyData::CONTROL_BLOCK:
        out.push_back(static_cast<uint8_t>(TAPROOT_LEAF_TAPSCRIPT | rng.randbits<1>()));
        Append(out, VALID_XONLY);
        Append(out, rng.randbytes<uint8_t>(32 * rng.randrange<size_t>(3)));
        break;
    case KeyData::XPUB:
        Append(out, XPUB_VERSION);
        // A nonzero depth, so that the random fingerprint and child number are allowed.
        out.push_back(static_cast<uint8_t>(1 + rng.randrange<uint8_t>(255)));
        Append(out, rng.randbytes<uint8_t>(4 + 4 + 32)); // fingerprint, child number, chaincode
        Append(out, RandomPubkey(rng));
        break;
    case KeyData::MUSIG2_IDS:
        Append(out, RandomPubkey(rng));
        Append(out, RandomPubkey(rng));
        if (rng.randbool()) Append(out, rng.randbytes<uint8_t>(32));
        break;
    case KeyData::PROPRIETARY:
        out = Ser(rng.randbytes<uint8_t>(rng.randrange<size_t>(8)), COMPACTSIZE(rng.randrange<uint64_t>(4)));
        Append(out, rng.randbytes<uint8_t>(rng.randrange<size_t>(8)));
        break;
    case KeyData::BYTES20:
        out = rng.randbytes<uint8_t>(20);
        break;
    case KeyData::BYTES32:
        out = rng.randbytes<uint8_t>(32);
        break;
    }
    return out;
}

std::vector<uint8_t> MakeValue(ValueKind kind, size_t len, InsecureRandomContext& rng)
{
    switch (kind) {
    case ValueKind::RANDOM:
        return rng.randbytes<uint8_t>(len);
    case ValueKind::TX: {
        CMutableTransaction tx;
        tx.version = 2;
        for (size_t i{0}, n{1 + rng.randrange<size_t>(3)}; i < n; ++i) {
            tx.vin.emplace_back(COutPoint{Txid::FromUint256(rng.rand256()), rng.rand32()});
        }
        for (size_t i{0}, n{1 + rng.randrange<size_t>(3)}; i < n; ++i) {
            const auto script{rng.randbytes<uint8_t>(len)};
            tx.vout.emplace_back(static_cast<CAmount>(rng.rand64()), CScript(script.begin(), script.end()));
        }
        return Ser(TX_NO_WITNESS(tx));
    }
    case ValueKind::TXOUT: {
        const auto script{rng.randbytes<uint8_t>(len)};
        return Ser(CTxOut{static_cast<CAmount>(rng.rand64()), CScript(script.begin(), script.end())});
    }
    case ValueKind::DER_SIG: {
        auto r{rng.randbytes<uint8_t>(32)};
        auto s{rng.randbytes<uint8_t>(32)};
        r[0] = (r[0] & 0x7f) | 0x01;
        s[0] = (s[0] & 0x7f) | 0x01;
        std::vector<uint8_t> out{0x30, 2 + 32 + 2 + 32, 0x02, 32};
        Append(out, r);
        Append(out, std::array<uint8_t, 2>{0x02, 32});
        Append(out, s);
        out.push_back(SIGHASH_ALL);
        return out;
    }
    case ValueKind::KEY_ORIGIN:
        return rng.randbytes<uint8_t>(4 * (1 + rng.randrange<size_t>(4)));
    case ValueKind::TAP_KEY_ORIGIN: {
        std::vector<uint256> leaf_hashes(rng.randrange<size_t>(3));
        for (uint256& hash : leaf_hashes) hash = rng.rand256();
        std::vector<uint8_t> out{Ser(leaf_hashes)};
        Append(out, rng.randbytes<uint8_t>(4 * (1 + rng.randrange<size_t>(4))));
        return out;
    }
    case ValueKind::PUBKEYS: {
        std::vector<uint8_t> out;
        for (size_t i{0}, n{1 + rng.randrange<size_t>(3)}; i < n; ++i) {
            Append(out, RandomPubkey(rng));
        }
        return out;
    }
    case ValueKind::TAP_TREE: {
        // Either a single leaf at depth 0, or two leaves at depth 1. Both are complete trees.
        const uint8_t depth{rng.randbool()};
        std::vector<uint8_t> out;
        for (int i{0}; i < 1 + depth; ++i) {
            Append(out, Ser(depth, TAPROOT_LEAF_TAPSCRIPT, rng.randbytes<uint8_t>(rng.randrange<size_t>(16))));
        }
        return out;
    }
    case ValueKind::PSBT_VERSION:
        return Ser(uint32_t{rng.randbool() ? 2U : 0U});
    }
    return {};
}

Record MakeKnownRecord(const KnownKey& k, InsecureRandomContext& rng)
{
    Record rec;
    rec.key = {k.type};
    Append(rec.key, MakeKeyData(k.key_data, rng));
    rec.value = MakeValue(k.value_kind, k.value_len ? k.value_len : rng.randrange<size_t>(64), rng);
    return rec;
}

//! Pick a well-known key for the map at map_idx, preferring keys of the PSBT version it declares.
const KnownKey& PickKnownKey(const std::vector<Map>& maps, size_t map_idx, InsecureRandomContext& rng)
{
    const uint8_t version{DetectVersion(maps.front())};
    const std::span<const KnownKey> pool{PoolFor(map_idx, InputCount(maps.front(), version), rng)};
    const uint8_t allowed{rng.randrange<int>(8) == 0 ? ANY_VERSION : version};
    while (true) {
        const KnownKey& k{pool[rng.randrange(pool.size())]};
        if (k.versions & allowed) return k;
    }
}

//! Apply one structure-aware mutation to maps in place.
void MutateMaps(std::vector<Map>& maps, InsecureRandomContext& rng)
{
    constexpr int NUM_MUTATION_KINDS{6};
    const size_t map_idx{rng.randrange(maps.size())};
    Map& map{maps[map_idx]};
    switch (rng.randrange<int>(NUM_MUTATION_KINDS)) {
    case 0: { // mutate an existing record's value bytes
        if (map.empty()) break;
        Record& rec{map[rng.randrange(map.size())]};
        const size_t old_size{rec.value.size()};
        rec.value.resize(old_size + 16);
        rec.value.resize(FuzzDefaultMutate(rec.value.data(), old_size, rec.value.size()));
        break;
    }
    case 1: { // mutate an existing record's key bytes
        if (map.empty()) break;
        Record& rec{map[rng.randrange(map.size())]};
        if (rec.key.empty()) break;
        // Leave room to grow, so keydata of other lengths can be reached.
        const size_t old_size{rec.key.size()};
        rec.key.resize(old_size + 16);
        rec.key.resize(FuzzDefaultMutate(rec.key.data(), old_size, rec.key.size()));
        break;
    }
    case 2: { // remove a record
        if (map.empty()) break;
        map.erase(map.begin() + rng.randrange(map.size()));
        break;
    }
    case 3: { // duplicate a record (exercises duplicate-key rejection paths)
        if (map.empty()) break;
        map.push_back(map[rng.randrange(map.size())]);
        break;
    }
    case 4: { // insert a well-known field for this kind of map
        map.push_back(MakeKnownRecord(PickKnownKey(maps, map_idx, rng), rng));
        break;
    }
    case 5: { // add or remove a whole map (grows/shrinks the apparent input/output count)
        if (maps.size() <= 1 || rng.randbool()) {
            maps.emplace_back();
        } else {
            maps.erase(maps.begin() + map_idx);
        }
        break;
    }
    }
}

} // namespace

size_t PSBTCustomMutator(uint8_t* data, size_t size, size_t max_size, unsigned int seed)
{
    InsecureRandomContext rng{seed};

    // Occasionally mutate the raw bytes instead, so that inputs with a malformed
    // container are still generated.
    if (rng.randrange<size_t>(8) == 0) return FuzzDefaultMutate(data, size, max_size);

    std::vector<Map> maps{ParsePSBT({data, size})};
    std::vector<uint8_t> out;
    if (maps.empty()) {
        out = rng.randbool() ? BuildPSBTv0Template() : BuildPSBTv2Template();
    } else {
        const size_t num_mutations{1 + rng.randrange<size_t>(3)};
        for (size_t i{0}; i < num_mutations; ++i) {
            MutateMaps(maps, rng);
        }
        out = SerializePSBT(maps);
    }
    out.resize(std::min(out.size(), max_size));
    std::ranges::copy(out, data);
    return out.size();
}
