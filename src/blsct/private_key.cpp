// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/private_key.h>

namespace blsct {

PrivateKey::PrivateKey(Scalar k_)
{
    k.resize(PrivateKey::SIZE);
    std::vector<unsigned char> v = k_.GetVch();
    memcpy(k.data(), &v.front(), k.size());
}

PrivateKey::PrivateKey(CPrivKey k_)
{
    k.resize(PrivateKey::SIZE);
    memcpy(k.data(), &k_.front(), k.size());
}

bool PrivateKey::operator==(const PrivateKey& rhs) const
{
    return k == rhs.k;
}

PrivateKey::Point PrivateKey::GetPoint() const
{
    return Point::GetBasePoint() * Scalar(std::vector<unsigned char>(k.begin(), k.end()));
}

PublicKey PrivateKey::GetPublicKey() const
{
    auto point = GetPoint();
    return point;
}

PrivateKey::Scalar PrivateKey::GetScalar() const
{
    auto ret = std::vector<unsigned char>(k.begin(), k.end());
    return ret;
}

bool PrivateKey::IsValid() const
{
    if (k.size() == 0) return false;
    Scalar s = GetScalar();
    return s.IsValid() && !s.IsZero();
}

void PrivateKey::SetToZero()
{
    k.clear();
}

Signature PrivateKey::CoreSign(const Message& msg) const
{
    blsSecretKey bls_sk { GetScalar().GetUnderlying() };

    Signature sig;
    blsSign(&sig.m_data, &bls_sk, &msg[0], msg.size());
    return sig;
}

Signature PrivateKey::SignBalance() const
{
    return CoreSign(Common::BLSCTBALANCE);
}

Signature PrivateKey::Sign(const uint256& msg) const
{
    return Sign(Message(msg.begin(), msg.end()));
}

Signature PrivateKey::Sign(const Message& msg) const
{
    auto pk = GetPublicKey();
    auto aug_msg = pk.AugmentMessage(msg);
    auto sig = CoreSign(aug_msg);
    return sig;
}

bool PrivateKey::VerifyPubKey(const PublicKey& pk) const
{
    return GetPublicKey() == pk;
}

} // namespace blsct
