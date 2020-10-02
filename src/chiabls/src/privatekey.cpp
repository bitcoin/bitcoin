// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include <cstring>
#include <algorithm>

#include "bls.hpp"
#include "util.hpp"
#include "privatekey.hpp"
namespace bls {
PrivateKey PrivateKey::FromSeed(const uint8_t* seed, size_t seedLen) {
    // "BLS private key seed" in ascii
    const uint8_t hmacKey[] = {66, 76, 83, 32, 112, 114, 105, 118, 97, 116, 101,
                              32, 107, 101, 121, 32, 115, 101, 101, 100};

    uint8_t* hash = Util::SecAlloc<uint8_t>(
            PrivateKey::PRIVATE_KEY_SIZE);

    // Hash the seed into sk
    md_hmac(hash, seed, seedLen, hmacKey, sizeof(hmacKey));

    bn_t order;
    bn_new(order);
    g1_get_ord(order);

    // Make sure private key is less than the curve order
    bn_t* skBn = Util::SecAlloc<bn_t>(1);
    bn_new(*skBn);
    bn_read_bin(*skBn, hash, PrivateKey::PRIVATE_KEY_SIZE);
    bn_mod_basic(*skBn, *skBn, order);

    PrivateKey k;
    bn_copy(*k.keydata, *skBn);

    Util::SecFree(skBn);
    Util::SecFree(hash);
    return k;
}

// Construct a private key from a bytearray.
PrivateKey PrivateKey::FromBytes(const uint8_t* bytes, bool modOrder) {
    PrivateKey k;
    bn_read_bin(*k.keydata, bytes, PrivateKey::PRIVATE_KEY_SIZE);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    if (modOrder) {
        bn_mod_basic(*k.keydata, *k.keydata, ord);
    } else {
        if (bn_cmp(*k.keydata, ord) > 0) {
            throw std::string("Key data too large, must be smaller than group order");
        }
    }
    return k;
}

PrivateKey::PrivateKey() {
    AllocateKeyData();
}

// Construct a private key from another private key.
PrivateKey::PrivateKey(const PrivateKey &privateKey) {
    AllocateKeyData();
    bn_copy(*keydata, *privateKey.keydata);
}

PrivateKey::PrivateKey(PrivateKey&& k) {
    std::swap(keydata, k.keydata);
}

PrivateKey::~PrivateKey() {
    Util::SecFree(keydata);
}

PublicKey PrivateKey::GetPublicKey() const {
    g1_t *q = Util::SecAlloc<g1_t>(1);
    g1_mul_gen(*q, *keydata);

    const PublicKey ret = PublicKey::FromG1(q);
    Util::SecFree(*q);
    return ret;
}

PrivateKey PrivateKey::AggregateInsecure(std::vector<PrivateKey> const& privateKeys) {
    if (privateKeys.empty()) {
        throw std::string("Number of private keys must be at least 1");
    }

    bn_t order;
    bn_new(order);
    g1_get_ord(order);

    PrivateKey ret(privateKeys[0]);
    for (size_t i = 1; i < privateKeys.size(); i++) {
        bn_add(*ret.keydata, *ret.keydata, *privateKeys[i].keydata);
        bn_mod_basic(*ret.keydata, *ret.keydata, order);
    }
    return ret;
}

PrivateKey PrivateKey::Aggregate(std::vector<PrivateKey> const& privateKeys,
                                       std::vector<PublicKey> const& pubKeys) {
    if (pubKeys.size() != privateKeys.size()) {
        throw std::string("Number of public keys must equal number of private keys");
    }
    if (privateKeys.empty()) {
        throw std::string("Number of keys must be at least 1");
    }

    std::vector<uint8_t*> serPubKeys(pubKeys.size());
    for (size_t i = 0; i < pubKeys.size(); i++) {
        serPubKeys[i] = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
        pubKeys[i].Serialize(serPubKeys[i]);
    }

    // Sort the public keys and private keys by public key
    std::vector<size_t> keysSorted(privateKeys.size());
    for (size_t i = 0; i < privateKeys.size(); i++) {
        keysSorted[i] = i;
    }

    std::sort(keysSorted.begin(), keysSorted.end(), [&serPubKeys](size_t a, size_t b) {
        return memcmp(serPubKeys[a], serPubKeys[b], PublicKey::PUBLIC_KEY_SIZE) < 0;
    });


    bn_t *computedTs = new bn_t[keysSorted.size()];
    for (size_t i = 0; i < keysSorted.size(); i++) {
        bn_new(computedTs[i]);
    }
    BLS::HashPubKeys(computedTs, keysSorted.size(), serPubKeys, keysSorted);

    // Raise all keys to power of the corresponding t's and aggregate the results into aggKey
    std::vector<PrivateKey> expKeys;
    expKeys.reserve(keysSorted.size());
    for (size_t i = 0; i < keysSorted.size(); i++) {
        auto& k = privateKeys[keysSorted[i]];
        expKeys.emplace_back(k.Mul(computedTs[i]));
    }
    PrivateKey aggKey = PrivateKey::AggregateInsecure(expKeys);

    for (size_t i = 0; i < keysSorted.size(); i++) {
        bn_free(p);
    }
    for (auto p : serPubKeys) {
        delete[] p;
    }
    delete[] computedTs;

    BLS::CheckRelicErrors();
    return aggKey;
}

PrivateKey PrivateKey::Mul(const bn_t n) const {
    bn_t order;
    bn_new(order);
    g2_get_ord(order);

    PrivateKey ret;
    bn_mul_comba(*ret.keydata, *keydata, n);
    bn_mod_basic(*ret.keydata, *ret.keydata, order);
    return ret;
}

bool operator==(const PrivateKey& a, const PrivateKey& b) {
    return bn_cmp(*a.keydata, *b.keydata) == CMP_EQ;
}

bool operator!=(const PrivateKey& a, const PrivateKey& b) {
    return !(a == b);
}

PrivateKey& PrivateKey::operator=(const PrivateKey &rhs) {
    Util::SecFree(keydata);
    AllocateKeyData();
    bn_copy(*keydata, *rhs.keydata);
    return *this;
}

void PrivateKey::Serialize(uint8_t* buffer) const {
    bn_write_bin(buffer, PrivateKey::PRIVATE_KEY_SIZE, *keydata);
}

std::vector<uint8_t> PrivateKey::Serialize() const {
    std::vector<uint8_t> data(PRIVATE_KEY_SIZE);
    Serialize(data.data());
    return data;
}

InsecureSignature PrivateKey::SignInsecure(const uint8_t *msg, size_t len) const {
    uint8_t messageHash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(messageHash, msg, len);
    return SignInsecurePrehashed(messageHash);
}

InsecureSignature PrivateKey::SignInsecurePrehashed(const uint8_t *messageHash) const {
    g2_t sig, point;

    g2_map(point, messageHash, BLS::MESSAGE_HASH_LEN, 0);
    g2_mul(sig, point, *keydata);

    return InsecureSignature::FromG2(&sig);
}

Signature PrivateKey::Sign(const uint8_t *msg, size_t len) const {
    uint8_t messageHash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(messageHash, msg, len);
    return SignPrehashed(messageHash);
}

Signature PrivateKey::SignPrehashed(const uint8_t *messageHash) const {
    InsecureSignature insecureSig = SignInsecurePrehashed(messageHash);
    Signature ret = Signature::FromInsecureSig(insecureSig);

    ret.SetAggregationInfo(AggregationInfo::FromMsgHash(GetPublicKey(),
            messageHash));

    return ret;
}

void PrivateKey::AllocateKeyData() {
    keydata = Util::SecAlloc<bn_t>(1);
    bn_new(*keydata);  // Freed in destructor
    bn_zero(*keydata);
}
} // end namespace bls
