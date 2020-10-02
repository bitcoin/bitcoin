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

#include <iostream>
#include <cstring>
#include <algorithm>

#include "publickey.hpp"
#include "util.hpp"
#include "bls.hpp"

namespace bls {
PublicKey PublicKey::FromBytes(const uint8_t * key) {
    PublicKey pk = PublicKey();
    uint8_t uncompressed[PUBLIC_KEY_SIZE + 1];
    std::memcpy(uncompressed + 1, key, PUBLIC_KEY_SIZE);
    if (key[0] & 0x80) {
        uncompressed[0] = 0x03;   // Insert extra byte for Y=1
        uncompressed[1] &= 0x7f;  // Remove initial Y bit
    } else {
        uncompressed[0] = 0x02;   // Insert extra byte for Y=0
    }
    g1_read_bin(pk.q, uncompressed, PUBLIC_KEY_SIZE + 1);
    BLS::CheckRelicErrors();
    return pk;
}

PublicKey PublicKey::FromG1(const g1_t* pubKey) {
    PublicKey pk = PublicKey();
    g1_copy(pk.q, *pubKey);
    return pk;
}

PublicKey::PublicKey() {
    g1_set_infty(q);
}

PublicKey::PublicKey(const PublicKey &pubKey) {
    g1_copy(q, pubKey.q);
}

PublicKey PublicKey::AggregateInsecure(std::vector<PublicKey> const& pubKeys) {
    if (pubKeys.empty()) {
        throw std::string("Number of public keys must be at least 1");
    }

    PublicKey ret = pubKeys[0];
    for (size_t i = 1; i < pubKeys.size(); i++) {
        g1_add(ret.q, ret.q, pubKeys[i].q);
    }
    return ret;
}

PublicKey PublicKey::Aggregate(std::vector<PublicKey> const& pubKeys) {
    if (pubKeys.size() < 1) {
        throw std::string("Number of public keys must be at least 1");
    }

    std::vector<uint8_t*> serPubKeys(pubKeys.size());
    for (size_t i = 0; i < pubKeys.size(); i++) {
        serPubKeys[i] = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
        pubKeys[i].Serialize(serPubKeys[i]);
    }

    // Sort the public keys by public key
    std::vector<size_t> pubKeysSorted(pubKeys.size());
    for (size_t i = 0; i < pubKeysSorted.size(); i++) {
        pubKeysSorted[i] = i;
    }

    std::sort(pubKeysSorted.begin(), pubKeysSorted.end(), [&serPubKeys](size_t a, size_t b) {
        return memcmp(serPubKeys[a], serPubKeys[b], PublicKey::PUBLIC_KEY_SIZE) < 0;
    });

    bn_t *computedTs = new bn_t[pubKeysSorted.size()];
    for (size_t i = 0; i < pubKeysSorted.size(); i++) {
        bn_new(computedTs[i]);
    }
    BLS::HashPubKeys(computedTs, pubKeysSorted.size(), serPubKeys, pubKeysSorted);

    // Raise all keys to power of the corresponding t's and aggregate the results into aggKey
    std::vector<PublicKey> expKeys;
    expKeys.reserve(pubKeysSorted.size());
    for (size_t i = 0; i < pubKeysSorted.size(); i++) {
        const PublicKey& pk = pubKeys[pubKeysSorted[i]];
        expKeys.emplace_back(pk.Exp(computedTs[i]));
    }
    PublicKey aggKey = PublicKey::AggregateInsecure(expKeys);

    for (size_t i = 0; i < pubKeysSorted.size(); i++) {
        bn_free(computedTs[i]);
    }
    for (auto p : serPubKeys) {
        delete[] p;
    }
    delete[] computedTs;

    BLS::CheckRelicErrors();
    return aggKey;
}

PublicKey PublicKey::Exp(bn_t const n) const {
    PublicKey ret;
    g1_mul(ret.q, q, n);
    return ret;
}

void PublicKey::Serialize(uint8_t *buffer) const {
    CompressPoint(buffer, &q);
}

std::vector<uint8_t> PublicKey::Serialize() const {
    std::vector<uint8_t> data(PUBLIC_KEY_SIZE);
    Serialize(data.data());
    return data;
}

// Comparator implementation.
bool operator==(PublicKey const &a,  PublicKey const &b) {
    return g1_cmp(a.q, b.q) == CMP_EQ;
}

bool operator!=(PublicKey const&a,  PublicKey const&b) {
    return !(a == b);
}

std::ostream &operator<<(std::ostream &os, PublicKey const &pk) {
    uint8_t data[PublicKey::PUBLIC_KEY_SIZE];
    pk.Serialize(data);
    return os << Util::HexStr(data, PublicKey::PUBLIC_KEY_SIZE);
}

uint32_t PublicKey::GetFingerprint() const {
    uint8_t buffer[PublicKey::PUBLIC_KEY_SIZE];
    uint8_t hash[32];
    Serialize(buffer);
    Util::Hash256(hash, buffer, PublicKey::PUBLIC_KEY_SIZE);
    return Util::FourBytesToInt(hash);
}

void PublicKey::CompressPoint(uint8_t* result, const g1_t* point) {
    uint8_t buffer[PublicKey::PUBLIC_KEY_SIZE + 1];
    g1_write_bin(buffer, PublicKey::PUBLIC_KEY_SIZE + 1, *point, 1);

    if (buffer[0] == 0x03) {
        buffer[1] |= 0x80;
    }
    std::memcpy(result, buffer + 1, PUBLIC_KEY_SIZE);
}
} // end namespace bls
