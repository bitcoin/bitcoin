// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/keys.h>

namespace blsct {

PublicKey PublicKey::Aggregate(std::vector<PublicKey> vPk)
{
    auto retPoint = G1Point();
    bool isZero = true;

    for (auto& pk : vPk) {
        G1Point pkG1;

        bool success = pk.GetG1Point(pkG1);
        if (!success)
            throw std::runtime_error(strprintf("%s: Vector of public keys has an invalid element", __func__));

        retPoint = isZero ? pkG1 : retPoint + pkG1;
        isZero = false;
    }

    return PublicKey(retPoint);
}

uint256 PublicKey::GetHash() const
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << data;
    return ss.GetHash();
}

CKeyID PublicKey::GetID() const
{
    return CKeyID(Hash160(data));
}

bool PublicKey::GetG1Point(G1Point& ret) const
{
    try {
        ret = G1Point(data);
    } catch (...) {
        return false;
    }
    return true;
}

std::string PublicKey::ToString() const
{
    return HexStr(GetVch());
};

bool PublicKey::operator==(const PublicKey& rhs) const
{
    return GetVch() == rhs.GetVch();
}

bool PublicKey::IsValid() const
{
    if (data.size() == 0) return false;

    G1Point g1;

    if (!GetG1Point(g1)) return false;

    return g1.IsValid();
}

std::vector<unsigned char> PublicKey::GetVch() const
{
    return data;
}

CKeyID DoublePublicKey::GetID() const
{
    return CKeyID(Hash160(GetVch()));
}

bool DoublePublicKey::GetViewKey(G1Point& ret) const
{
    try {
        ret = G1Point(vk.GetVch());
    } catch (...) {
        return false;
    }

    return true;
}

bool DoublePublicKey::GetSpendKey(G1Point& ret) const
{
    try {
        ret = G1Point(sk.GetVch());
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

bool PrivateKey::operator==(const PrivateKey& rhs) const
{
    return k == rhs.k;
}

G1Point PrivateKey::GetG1Point() const
{
    return G1Point::GetBasePoint() * Scalar(std::vector<unsigned char>(k.begin(), k.end()));
}

PublicKey PrivateKey::GetPublicKey() const
{
    return PublicKey(GetG1Point());
}

Scalar PrivateKey::GetScalar() const
{
    return Scalar(std::vector<unsigned char>(k.begin(), k.end()));
}

bool PrivateKey::IsValid() const
{
    if (k.size() == 0) return false;
    return GetScalar().IsValid();
}

void PrivateKey::SetToZero()
{
    k.clear();
}
} // namespace blsct
