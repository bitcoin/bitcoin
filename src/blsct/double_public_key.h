// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_DOUBLE_PUBLIC_KEY_H
#define NAVCOIN_BLSCT_DOUBLE_PUBLIC_KEY_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/public_key.h>

namespace blsct {

class DoublePublicKey
{
private:
    using Point = MclG1Point;

    PublicKey vk;
    PublicKey sk;
    bool is_fully_built = false;

public:
    static constexpr size_t SIZE = blsct::PublicKey::SIZE * 2;

    DoublePublicKey() : is_fully_built(true) {}
    DoublePublicKey(const PublicKey& vk_, const PublicKey& sk_) : vk(vk_), sk(sk_), is_fully_built(true) {}
    DoublePublicKey(const Point& vk_, const Point& sk_) : vk(vk_), sk(sk_), is_fully_built(true) {}

    DoublePublicKey(const std::vector<unsigned char>& vk_, const std::vector<unsigned char>& sk_) : vk(vk_), sk(sk_)
    {
        MclG1Point p;
        is_fully_built = p.SetVch(vk_) && p.SetVch(sk_);
    }

    DoublePublicKey(const std::vector<unsigned char>& keys);

    SERIALIZE_METHODS(DoublePublicKey, obj) { READWRITE(obj.vk, obj.sk); }

    uint256 GetHash() const;
    CKeyID GetID() const;

    bool GetViewKey(Point& ret) const;
    bool GetSpendKey(Point& ret) const;

    bool GetViewKey(PublicKey& ret) const;
    bool GetSpendKey(PublicKey& ret) const;

    bool operator==(const DoublePublicKey& rhs) const;
    bool operator<(const DoublePublicKey& rhs) const;

    bool IsValid() const;

    std::vector<unsigned char> GetVkVch() const;
    std::vector<unsigned char> GetSkVch() const;
    std::vector<unsigned char> GetVch() const;
};

} // namespace blsct

#endif // NAVCOIN_BLSCT_DOUBLE_PUBLIC_KEY_H
