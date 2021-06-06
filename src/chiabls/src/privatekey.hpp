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

#ifndef SRC_BLSPRIVATEKEY_HPP_
#define SRC_BLSPRIVATEKEY_HPP_

#include "relic_conf.h"

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

#include "elements.hpp"

namespace bls {
class PrivateKey {
 public:
    // Private keys are represented as 32 byte field elements. Note that
    // not all 32 byte integers are valid keys, the private key must be
    // less than the group order (which is in bls.hpp).
    static const size_t PRIVATE_KEY_SIZE = 32;

    // Construct a private key from a bytearray.
    static PrivateKey FromBytes(const Bytes& bytes, bool modOrder = false);

    // Construct a private key from a bytearray.
    static PrivateKey FromByteVector(const std::vector<uint8_t> bytes, bool modOrder = false);

    // Aggregate many private keys into one (sum of keys mod order)
    static PrivateKey Aggregate(std::vector<PrivateKey> const &privateKeys);

    PrivateKey();

    // Construct a private key from another private key. Allocates memory in
    // secure heap, and copies keydata.
    PrivateKey(const PrivateKey &k);
    PrivateKey(PrivateKey &&k);

    PrivateKey& operator=(const PrivateKey& other);
    PrivateKey& operator=(PrivateKey&& other);

    ~PrivateKey();

    const G1Element& GetG1Element() const;
    const G2Element& GetG2Element() const;

    G2Element GetG2Power(const G2Element& element) const;

    bool IsZero() const;

    // Compare to different private key
    friend bool operator==(const PrivateKey &a, const PrivateKey &b);
    friend bool operator!=(const PrivateKey &a, const PrivateKey &b);

    // Multiply private key by G1 or G2 elements
    friend G1Element operator*(const G1Element &a, const PrivateKey &k);
    friend G1Element operator*(const PrivateKey &k, const G1Element &a);

    friend G2Element operator*(const G2Element &a, const PrivateKey &k);
    friend G2Element operator*(const PrivateKey &k, const G2Element &a);

    friend PrivateKey operator*(const PrivateKey& a, const bn_t& k);
    friend PrivateKey operator*(const bn_t& k, const PrivateKey& a);

    // Serialize the key into bytes
    void Serialize(uint8_t *buffer) const;
    std::vector<uint8_t> Serialize(bool fLegacy = false) const;

    G2Element SignG2(
        const uint8_t *msg,
        size_t len,
        const uint8_t *dst,
        size_t dst_len,
        bool fLegacy = false) const;
    
 private:

    // Allocate memory for private key
    void AllocateKeyData();
    /// Throw an error if keydata isn't initialized
    void CheckKeyData() const;
    /// Deallocate *keydata and keydata if requried
    void DeallocateKeyData();

    void InvalidateCaches();

    // The actual byte data
    bn_st* keydata{nullptr};

    mutable bool fG1CacheValid{false};
    mutable G1Element g1Cache;

    mutable bool fG2CacheValid{false};
    mutable G2Element g2Cache;
};
}  // end namespace bls

#endif  // SRC_BLSPRIVATEKEY_HPP_
