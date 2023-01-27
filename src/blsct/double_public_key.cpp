// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/double_public_key.h>

namespace blsct {

CKeyID DoublePublicKey::GetID() const
{
    return CKeyID(Hash160(GetVch()));
}

bool DoublePublicKey::GetViewKey(Point& ret) const
{
    try {
        ret = Point(vk.GetVch());
    } catch (...) {
        return false;
    }

    return true;
}

bool DoublePublicKey::GetSpendKey(Point& ret) const
{
    try {
        ret = Point(sk.GetVch());
    } catch (...) {
        return false;
    }

    return true;
}

bool DoublePublicKey::operator==(const DoublePublicKey& rhs) const
{
    return vk == rhs.vk && sk == rhs.sk;
}

bool DoublePublicKey::IsValid() const
{
    return vk.IsValid() && sk.IsValid();
}

std::vector<unsigned char> DoublePublicKey::GetVkVch() const
{
    return vk.GetVch();
}

std::vector<unsigned char> DoublePublicKey::GetSkVch() const
{
    return sk.GetVch();
}

std::vector<unsigned char> DoublePublicKey::GetVch() const
{
    auto ret = vk.GetVch();
    auto toAppend = sk.GetVch();

    ret.insert(ret.end(), toAppend.begin(), toAppend.end());

    return ret;
}

}  // namespace blsct
