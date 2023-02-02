// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_PRIVATE_KEY_H
#define NAVCOIN_BLSCT_PRIVATE_KEY_H

#define BLS_ETH 1

#include <blsct/arith/mcl/mcl.h>
#include <blsct/public_key.h>
#include <blsct/bls12_381_common.h>
#include <key.h>

namespace blsct {

class PrivateKey
{
private:
    using Point = MclG1Point;
    using Scalar = MclScalar;

    CPrivKey k;

public:
    static constexpr size_t SIZE = 32;

    PrivateKey() { k.clear(); }
    PrivateKey(Scalar k_);
    PrivateKey(CPrivKey k_);

    SERIALIZE_METHODS(PrivateKey, obj) { READWRITE(std::vector<unsigned char>(obj.k.begin(), obj.k.end())); }

    bool operator==(const PrivateKey& rhs) const;

    Point GetPoint() const;
    PublicKey GetPublicKey() const;
    Scalar GetScalar() const;
    bool IsValid() const;
    void SetToZero();

    // Basic scheme
    Signature SignBalance() const;

    // Message augmentation scheme
    Signature Sign(const Message& msg) const;

    // Core operations
    Signature CoreSign(const Message& msg) const;

    friend class CCryptoKeyStore;
    friend class CBasicKeyStore;
};

}

#endif  // NAVCOIN_BLSCT_PRIVATE_KEY_H
