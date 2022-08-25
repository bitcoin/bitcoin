// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/keys.h>

namespace blsct
{
CKeyID DoublePublicKey::GetID() const
{
    return CKeyID(Hash160(sk.GetVch()));
}

bool DoublePublicKey::GetViewKey(G1Point &ret) const {
    try
    {
        ret = G1Point(vk.GetVch());
    }
    catch(...)
    {
        return false;
    }

    return true;
}

bool DoublePublicKey::GetSpendKey(G1Point &ret) const {
    try
    {
        ret = G1Point(sk.GetVch());
    }
    catch(...)
    {
        return false;
    }

    return true;
}

bool DoublePublicKey::operator==(const DoublePublicKey& rhs) const {
    return vk==rhs.vk && sk==rhs.sk;
}

bool DoublePublicKey::IsValid() const {
    return vk.IsValid() && sk.IsValid();
}

std::vector<uint8_t> DoublePublicKey::GetVkVch() const {
    return vk.GetVch();
}

std::vector<uint8_t> DoublePublicKey::GetSkVch() const {
    return sk.GetVch();
}

uint256 PublicKey::GetHash() const {
    CHashWriter ss(SER_GETHASH, 0);
    ss << data;
    return ss.GetHash();
}

CKeyID PublicKey::GetID() const
{
    return CKeyID(Hash160(data));
}

bool PublicKey::GetG1Point(G1Point& ret) const {
    try
    {
        ret = G1Point(data);
    }
    catch(...)
    {
        return false;
    }
    return true;
}

bool PublicKey::operator==(const PublicKey& rhs) const {
    return GetVch()==rhs.GetVch();
}

bool PublicKey::IsValid() const {
    return data.size() > 0;
}

std::vector<uint8_t> PublicKey::GetVch() const {
    return data;
}
}
