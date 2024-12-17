// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bls.hpp"
#include "legacy.hpp"

namespace bls {

const size_t PrivateKey::PRIVATE_KEY_SIZE;

PrivateKey PrivateKey::FromSeedBIP32(const Bytes& seed) {
    // "BLS private key seed" in ascii
    const uint8_t hmacKey[] = {66, 76, 83, 32, 112, 114, 105, 118, 97, 116, 101,
                               32, 107, 101, 121, 32, 115, 101, 101, 100};

    auto* hash = Util::SecAlloc<uint8_t>(
        PrivateKey::PRIVATE_KEY_SIZE);

    // Hash the seed into sk
    md_hmac(hash, seed.begin(), (int)seed.size(), hmacKey, sizeof(hmacKey));

    bn_t order;
    bn_new(order);
    g1_get_ord(order);

    // Make sure private key is less than the curve order
    bn_t* skBn = Util::SecAlloc<bn_t>(1);
    bn_new(*skBn);
    bn_read_bin(*skBn, hash, PrivateKey::PRIVATE_KEY_SIZE);
    bn_mod_basic(*skBn, *skBn, order);

    PrivateKey k;
    bn_copy(k.keydata, *skBn);

    Util::SecFree(skBn);
    Util::SecFree(hash);
    return k;
}

// Construct a private key from a bytearray.
PrivateKey PrivateKey::FromBytes(const Bytes& bytes, bool modOrder)
{
    if (bytes.size() != PRIVATE_KEY_SIZE) {
        throw std::invalid_argument("PrivateKey::FromBytes: Invalid size");
    }

    PrivateKey k;
    bn_read_bin(k.keydata, bytes.begin(), PrivateKey::PRIVATE_KEY_SIZE);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    if (modOrder) {
        bn_mod_basic(k.keydata, k.keydata, ord);
    } else {
        if (bn_cmp(k.keydata, ord) > 0) {
            throw std::invalid_argument(
                "PrivateKey byte data must be less than the group order");
        }
    }
    return k;
}

// Construct a private key from a bytearray.
PrivateKey PrivateKey::FromByteVector(const std::vector<uint8_t> bytes, bool modOrder)
{
    return PrivateKey::FromBytes(Bytes(bytes), modOrder);
}

// Construct a private key from a bytearray.
PrivateKey PrivateKey::RandomPrivateKey()
{
    bn_t *r = Util::SecAlloc<bn_t>(1);
    bn_new(*r);
    bn_rand(*r, RLC_POS, 256);
    PrivateKey k;
    bn_copy(k.keydata, *r);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod_basic(k.keydata, k.keydata, ord);
    Util::SecFree(r);
    return k;
}

PrivateKey::PrivateKey() {
    AllocateKeyData();
};

// Construct a private key from another private key.
PrivateKey::PrivateKey(const PrivateKey &privateKey)
{
    privateKey.CheckKeyData();
    AllocateKeyData();
    bn_copy(keydata, privateKey.keydata);
}

PrivateKey::PrivateKey(PrivateKey &&k)
    : keydata(std::exchange(k.keydata, nullptr))
{
    k.InvalidateCaches();
}

PrivateKey::~PrivateKey()
{
    DeallocateKeyData();
}

void PrivateKey::DeallocateKeyData()
{
    if(keydata != nullptr) {
        Util::SecFree(keydata);
        keydata = nullptr;
    }
    InvalidateCaches();
}

void PrivateKey::InvalidateCaches()
{
    fG1CacheValid = false;
    fG2CacheValid = false;
}

PrivateKey& PrivateKey::operator=(const PrivateKey& other)
{
    CheckKeyData();
    other.CheckKeyData();
    InvalidateCaches();
    bn_copy(keydata, other.keydata);
    return *this;
}

PrivateKey& PrivateKey::operator=(PrivateKey&& other)
{
    DeallocateKeyData();
    keydata = std::exchange(other.keydata, nullptr);
    other.InvalidateCaches();
    return *this;
}

const G1Element& PrivateKey::GetG1Element() const
{
    if (!fG1CacheValid) {
        CheckKeyData();
        g1_st *p = Util::SecAlloc<g1_st>(1);
        g1_mul_gen(p, keydata);

        g1Cache = G1Element::FromNative(p);
        Util::SecFree(p);
        fG1CacheValid = true;
    }
    return g1Cache;
}

const G2Element& PrivateKey::GetG2Element() const
{
    if (!fG2CacheValid) {
        CheckKeyData();
        g2_st *q = Util::SecAlloc<g2_st>(1);
        g2_mul_gen(q, keydata);

        g2Cache = G2Element::FromNative(q);
        Util::SecFree(q);
        fG2CacheValid = true;
    }
    return g2Cache;
}

bool PrivateKey::HasKeyData() const
{
    return (keydata != nullptr);
}

G1Element operator*(const G1Element &a, const PrivateKey &k)
{
    k.CheckKeyData();
    g1_st* ans = Util::SecAlloc<g1_st>(1);
    a.ToNative(ans);
    g1_mul(ans, ans, k.keydata);
    G1Element ret = G1Element::FromNative(ans);
    Util::SecFree(ans);
    return ret;
}

G1Element operator*(const PrivateKey &k, const G1Element &a) { return a * k; }

G2Element operator*(const G2Element &a, const PrivateKey &k)
{
    k.CheckKeyData();
    g2_st* ans = Util::SecAlloc<g2_st>(1);
    a.ToNative(ans);
    g2_mul(ans, ans, k.keydata);
    G2Element ret = G2Element::FromNative(ans);
    Util::SecFree(ans);
    return ret;
}

G2Element operator*(const PrivateKey &k, const G2Element &a) { return a * k; }

PrivateKey operator*(const PrivateKey& k, const bn_t& a)
{
    k.CheckKeyData();
    bn_t order;
    bn_new(order);
    g2_get_ord(order);

    PrivateKey ret;
    bn_mul_comba(ret.keydata, k.keydata, a);
    bn_mod_basic(ret.keydata, ret.keydata, order);
    return ret;
}

PrivateKey operator*(const bn_t& a, const PrivateKey& k) { return a * k; }

G2Element PrivateKey::GetG2Power(const G2Element& element) const
{
    CheckKeyData();
    g2_st* q = Util::SecAlloc<g2_st>(1);
    element.ToNative(q);
    g2_mul(q, q, keydata);

    const G2Element ret = G2Element::FromNative(q);
    Util::SecFree(q);
    return ret;
}

PrivateKey PrivateKey::Aggregate(std::vector<PrivateKey> const &privateKeys)
{
    if (privateKeys.empty()) {
        throw std::length_error("Number of private keys must be at least 1");
    }

    bn_t order;
    bn_new(order);
    g1_get_ord(order);

    PrivateKey ret;
    assert(ret.IsZero());
    for (size_t i = 0; i < privateKeys.size(); i++) {
        privateKeys[i].CheckKeyData();
        bn_add(ret.keydata, ret.keydata, privateKeys[i].keydata);
        bn_mod_basic(ret.keydata, ret.keydata, order);
    }
    return ret;
}

bool PrivateKey::IsZero() const {
    CheckKeyData();
    return bn_is_zero(keydata);
}

bool operator==(const PrivateKey &a, const PrivateKey &b)
{
    a.CheckKeyData();
    b.CheckKeyData();
    return bn_cmp(a.keydata, b.keydata) == RLC_EQ;
}

bool operator!=(const PrivateKey &a, const PrivateKey &b) { return !(a == b); }

void PrivateKey::Serialize(uint8_t *buffer) const
{
    if (buffer == nullptr) {
        throw std::runtime_error("PrivateKey::Serialize buffer invalid");
    }
    CheckKeyData();
    bn_write_bin(buffer, PrivateKey::PRIVATE_KEY_SIZE, keydata);
}

std::vector<uint8_t> PrivateKey::Serialize(const bool fLegacy) const
{
    std::vector<uint8_t> data(PRIVATE_KEY_SIZE);
    Serialize(data.data());
    return data;
}

std::array<uint8_t, PrivateKey::PRIVATE_KEY_SIZE> PrivateKey::SerializeToArray(bool fLegacy) const
{
    std::array<uint8_t, PRIVATE_KEY_SIZE> data{};
    Serialize(data.data());
    return data;
}

G2Element PrivateKey::SignG2(
    const uint8_t *msg,
    size_t len,
    const uint8_t *dst,
    size_t dst_len,
    const bool fLegacy) const
{
    CheckKeyData();

    g2_st* pt = Util::SecAlloc<g2_st>(1);

    if (fLegacy) {
        ep2_map_legacy(pt, msg, BLS::MESSAGE_HASH_LEN);
    } else {
        ep2_map_dst(pt, msg, len, dst, dst_len);
    }
    
    g2_mul(pt, pt, keydata);
    G2Element ret = G2Element::FromNative(pt);
    Util::SecFree(pt);
    return ret;
}

void PrivateKey::AllocateKeyData()
{
    assert(!keydata);
    keydata = Util::SecAlloc<bn_st>(1);
    keydata->alloc = RLC_BN_SIZE;
    bn_zero(keydata);
}

void PrivateKey::CheckKeyData() const
{
    if (keydata == nullptr) {
        throw std::runtime_error("PrivateKey::CheckKeyData keydata not initialized");
    }
}

}  // end namespace bls
