// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_PRIVATE_KEY_H
#define NAVCOIN_BLSCT_PRIVATE_KEY_H

#define BLS_ETH 1

#include <blsct/arith/mcl/mcl.h>
#include <blsct/public_key.h>
#include <blsct/common.h>
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

    PrivateKey() { k.resize(SIZE); }
    PrivateKey(Scalar k_);
    PrivateKey(CPrivKey k_);

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s.write(MakeByteSpan(k));
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s.read(MakeWritableByteSpan(k));
    }

    bool operator==(const PrivateKey& rhs) const;

    Point GetPoint() const;
    PublicKey GetPublicKey() const;
    Scalar GetScalar() const;
    bool IsValid() const;
    void SetToZero();
    bool VerifyPubKey(const PublicKey& pk) const;

    // Basic scheme
    Signature SignBalance() const;

    // Message augmentation scheme
    Signature Sign(const uint256& msg) const;
    Signature Sign(const Message& msg) const;

    // Core operations
    Signature CoreSign(const Message& msg) const;

    //! Simple read-only vector-like interface.
    unsigned int size() const { return (IsValid() ? k.size() : 0); }
    const std::byte* data() const { return reinterpret_cast<const std::byte*>(k.data()); }
    const unsigned char* begin() const { return k.data(); }
    const unsigned char* end() const { return k.data() + size(); }
};

}

#endif  // NAVCOIN_BLSCT_PRIVATE_KEY_H
