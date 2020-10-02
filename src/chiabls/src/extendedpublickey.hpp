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

#ifndef SRC_EXTENDEDPUBLICKEY_HPP_
#define SRC_EXTENDEDPUBLICKEY_HPP_

#include "relic_conf.h"

#include <vector>

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

#include "publickey.hpp"
#include "chaincode.hpp"


#include "relic.h"
#include "relic_test.h"

namespace bls {

/*
Defines a BIP-32 style node, which is composed of a private key and a
chain code. This follows the spec from BIP-0032, with a few changes:
  * The master secret key is generated mod n from the master seed,
    since not all 32 byte sequences are valid BLS private keys
  * Instead of SHA512(input), do SHA256(input || 00000000) ||
    SHA256(input || 00000001)
  * Mod n for the output of key derivation.
  * ID of a key is SHA256(pk) instead of HASH160(pk)
  * Serialization of extended public key is 93 bytes
*/
class ExtendedPublicKey {
 public:
    static const uint32_t VERSION = 1;

    // version(4) depth(1) parent fingerprint(4) child#(4) cc(32) pk(48)
    static const uint32_t EXTENDED_PUBLIC_KEY_SIZE = 93;

    // Parse public key and chain code from bytes
    static ExtendedPublicKey FromBytes(const uint8_t* serialized);

    // Derive a child extended public key, cannot be hardened
    ExtendedPublicKey PublicChild(uint32_t i) const;

    uint32_t GetVersion() const;
    uint8_t GetDepth() const;
    uint32_t GetParentFingerprint() const;
    uint32_t GetChildNumber() const;

    ChainCode GetChainCode() const;
    PublicKey GetPublicKey() const;

    // Comparator implementation.
    friend bool operator==(ExtendedPublicKey const &a,
                           ExtendedPublicKey const &b);
    friend bool operator!=(ExtendedPublicKey const &a,
                           ExtendedPublicKey const &b);
    friend std::ostream &operator<<(std::ostream &os,
                                    ExtendedPublicKey const &s);

    void Serialize(uint8_t *buffer) const;
    std::vector<uint8_t> Serialize() const;

 private:
    // private constructor, force use of static methods
    explicit ExtendedPublicKey(const uint32_t v, const uint8_t d,
                               const uint32_t pfp, const uint32_t cn,
                               const ChainCode code, const PublicKey key)
         : version(v),
          depth(d),
          parentFingerprint(pfp),
          childNumber(cn),
          chainCode(code),
          pk(key) {}

    const uint32_t version;
    const uint8_t depth;
    const uint32_t parentFingerprint;
    const uint32_t childNumber;

    const ChainCode chainCode;
    const PublicKey pk;
};
} // end namespace bls

#endif  // SRC_EXTENDEDPUBLICKEY_HPP_
