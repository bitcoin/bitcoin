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

#ifndef SRC_BLSPUBLICKEY_HPP_
#define SRC_BLSPUBLICKEY_HPP_

#include <iostream>
#include <vector>

#include "relic_conf.h"

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

#include "util.hpp"
namespace bls {
/** An encapsulated public key. */
class PublicKey {
 friend class InsecureSignature;
 friend class Signature;
 friend class ExtendedPublicKey;
 template <typename T> friend struct PolyOps;
 friend class BLS;
 public:
    static const size_t PUBLIC_KEY_SIZE = 48;

    // Construct a public key from a byte vector.
    static PublicKey FromBytes(const uint8_t* key);

    // Construct a public key from a native g1 element.
    static PublicKey FromG1(const g1_t* key);

    // Construct a public key from another public key.
    PublicKey(const PublicKey &pubKey);

    // Insecurely aggregate multiple public keys into one
    static PublicKey AggregateInsecure(std::vector<PublicKey> const& pubKeys);

    // Securely aggregate multiple public keys into one by exponentiating the keys with the pubKey hashes first
    static PublicKey Aggregate(std::vector<PublicKey> const& pubKeys);

    // Comparator implementation.
    friend bool operator==(PublicKey const &a,  PublicKey const &b);
    friend bool operator!=(PublicKey const &a,  PublicKey const &b);
    friend std::ostream &operator<<(std::ostream &os, PublicKey const &s);

    void Serialize(uint8_t *buffer) const;
    std::vector<uint8_t> Serialize() const;

    // Returns the first 4 bytes of the serialized pk
    uint32_t GetFingerprint() const;

 public:
    // Don't allow public construction, force static methods
    PublicKey();

 private:
    // Exponentiate public key with n
    PublicKey Exp(const bn_t n) const;

    static void CompressPoint(uint8_t* result, const g1_t* point);

 private:
    // Public key group element
    g1_t q;
};

} // end namespace bls
#endif  // SRC_BLSPUBLICKEY_HPP_
