// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_PUBLIC_KEY_H
#define NAVCOIN_BLSCT_PUBLIC_KEY_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/signature.h>
#include <key.h>

namespace blsct {
class PublicKey
{
public:
    using Point = MclG1Point;
    using Message = std::vector<uint8_t>;

private:
    Point point;

public:
    static constexpr size_t SIZE = 48;

    int8_t fValid = -1;

    PublicKey() {}
    PublicKey(const Point& pk)
    {
        try {
            point = Point{pk};
            fValid = point.IsValid() ? 1 : 0;
        } catch (...) {
            fValid = 0;
        }
    }
    PublicKey(const std::vector<unsigned char>& pk)
    {
        fValid = point.SetVch(pk);
    }

    SERIALIZE_METHODS(PublicKey, obj)
    {
        READWRITE(obj.point);
    }

    uint256 GetHash() const;
    CKeyID GetID() const;

    std::string ToString() const;

    bool operator==(const PublicKey& rhs) const;
    bool operator!=(const PublicKey& rhs) const;

    bool IsValid() const;

    Point GetG1Point() const;
    std::vector<unsigned char> GetVch() const;

    bool operator<(const PublicKey& b) const
    {
        return this->GetVch() < b.GetVch();
    };

    blsPublicKey ToBlsPublicKey() const;
    std::vector<uint8_t> AugmentMessage(const Message& msg) const;

    // Core operations
    bool CoreVerify(const Message& msg, const Signature& sig) const;

    // Basic scheme
    bool VerifyBalance(const Signature& sig) const;

    // Message augmentation scheme
    bool Verify(const Message& msg, const Signature& sig) const;
};

} // namespace blsct

#endif // NAVCOIN_BLSCT_PUBLIC_KEY_H
