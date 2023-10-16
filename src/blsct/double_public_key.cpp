// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/double_public_key.h>

namespace blsct {

DoublePublicKey::DoublePublicKey(const std::vector<unsigned char>& keys)
{
    if (keys.size() != SIZE) return;
    std::vector<unsigned char> vkData(SIZE / 2);
    std::vector<unsigned char> skData(SIZE / 2);
    std::copy(keys.begin(), keys.begin() + SIZE / 2, vkData.begin());
    std::copy(keys.begin() + SIZE / 2, keys.end(), skData.begin());
    vk = vkData;
    sk = skData;
}

CKeyID DoublePublicKey::GetID() const
{
    return sk.GetID();
}

bool DoublePublicKey::GetViewKey(PublicKey& ret) const
{
    ret = vk;
    return true;
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

bool DoublePublicKey::GetSpendKey(PublicKey& ret) const
{
    ret = sk;
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

bool DoublePublicKey::operator<(const DoublePublicKey& rhs) const
{
    return this->GetVkVch() == rhs.GetVkVch() ? this->GetSkVch() < rhs.GetSkVch() : this->GetVkVch() < rhs.GetVkVch();
};


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

} // namespace blsct
