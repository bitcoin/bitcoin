// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_PUBLIC_KEY_H
#define NAVCOIN_BLSCT_PUBLIC_KEY_H

#define BLS_ETH 1

#include <blsct/arith/mcl/mcl.h>
#include <blsct/signature.h>
#include <key.h>

namespace blsct {

class PublicKey
{
private:
    std::vector<unsigned char> data;

public:
    using Point = MclG1Point;
    using Message = std::vector<uint8_t>;

    static constexpr size_t SIZE = 48;

    PublicKey() { data.clear(); }
    PublicKey(const Point& pk) : data(pk.GetVch()) {}
    PublicKey(const std::vector<unsigned char>& pk) : data(pk) {}

    SERIALIZE_METHODS(PublicKey, obj) { READWRITE(obj.data); }

    uint256 GetHash() const;
    CKeyID GetID() const;

    std::string ToString() const;

    bool operator==(const PublicKey& rhs) const;

    bool IsValid() const;

    bool GetG1Point(Point& ret) const;
    std::vector<unsigned char> GetVch() const;

    blsPublicKey ToBlsPublicKey() const;
    std::vector<uint8_t> AugmentMessage(const Message& msg) const;

    // Core operations
    bool CoreVerify(const Message& msg, const Signature& sig) const;

    // Basic scheme
    bool VerifyBalance(const Signature& sig) const;

    // Message augmentation scheme
    bool Verify(const Message& msg, const Signature& sig) const;
};

}

#endif  // NAVCOIN_BLSCT_PUBLIC_KEY_H
