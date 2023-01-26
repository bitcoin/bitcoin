// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/keys.h>

namespace blsct {

template <typename T>
PublicKey<T> PublicKey<T>::Aggregate(std::vector<PublicKey<T>> vPk)
{
    using Point = typename T::Point;

    auto retPoint = Point();
    bool isZero = true;

    for (auto& pk : vPk) {
        Point pkG1;

        bool success = pk.GetG1Point(pkG1);
        if (!success)
            throw std::runtime_error(strprintf("%s: Vector of public keys has an invalid element", __func__));

        retPoint = isZero ? pkG1 : retPoint + pkG1;
        isZero = false;
    }

    return PublicKey(retPoint);
}
template PublicKey<Mcl> PublicKey<Mcl>::Aggregate(std::vector<PublicKey<Mcl>>);

template <typename T>
uint256 PublicKey<T>::GetHash() const
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << data;
    return ss.GetHash();
}
template uint256 PublicKey<Mcl>::GetHash() const;

template <typename T>
CKeyID PublicKey<T>::GetID() const
{
    return CKeyID(Hash160(data));
}
template CKeyID PublicKey<Mcl>::GetID() const;

template <typename T>
bool PublicKey<T>::GetG1Point(typename T::Point& ret) const
{
    try {
        ret = typename T::Point(data);
    } catch (...) {
        return false;
    }
    return true;
}
template bool PublicKey<Mcl>::GetG1Point(Mcl::Point& ret) const;

template <typename T>
std::string PublicKey<T>::ToString() const
{
    return HexStr(GetVch());
};
template std::string PublicKey<Mcl>::ToString() const;

template <typename T>
bool PublicKey<T>::operator==(const PublicKey<T>& rhs) const
{
    return GetVch() == rhs.GetVch();
}
template bool PublicKey<Mcl>::operator==(const PublicKey<Mcl>& rhs) const;

template <typename T>
bool PublicKey<T>::IsValid() const
{
    if (data.size() == 0) return false;

    typename T::Point g1;

    if (!GetG1Point(g1)) return false;

    return g1.IsValid();
}
template bool PublicKey<Mcl>::IsValid() const;

template <typename T>
std::vector<unsigned char> PublicKey<T>::GetVch() const
{
    return data;
}
template std::vector<unsigned char> PublicKey<Mcl>::GetVch() const;

template <typename T>
CKeyID DoublePublicKey<T>::GetID() const
{
    return CKeyID(Hash160(GetVch()));
}
template CKeyID DoublePublicKey<Mcl>::GetID() const;

template <typename T>
bool DoublePublicKey<T>::GetViewKey(typename T::Point& ret) const
{
    try {
        ret = typename T::Point(vk.GetVch());
    } catch (...) {
        return false;
    }

    return true;
}
template bool DoublePublicKey<Mcl>::GetViewKey(Mcl::Point& ret) const;

template <typename T>
bool DoublePublicKey<T>::GetSpendKey(typename T::Point& ret) const
{
    try {
        ret = typename T::Point(sk.GetVch());
    } catch (...) {
        return false;
    }

    return true;
}
template bool DoublePublicKey<Mcl>::GetSpendKey(typename Mcl::Point& ret) const;

template <typename T>
bool DoublePublicKey<T>::operator==(const DoublePublicKey<T>& rhs) const
{
    return vk == rhs.vk && sk == rhs.sk;
}
template bool DoublePublicKey<Mcl>::operator==(const DoublePublicKey<Mcl>& rhs) const;

template <typename T>
bool DoublePublicKey<T>::IsValid() const
{
    return vk.IsValid() && sk.IsValid();
}
template bool DoublePublicKey<Mcl>::IsValid() const;

template <typename T>
std::vector<unsigned char> DoublePublicKey<T>::GetVkVch() const
{
    return vk.GetVch();
}
template std::vector<unsigned char> DoublePublicKey<Mcl>::GetVkVch() const;

template <typename T>
std::vector<unsigned char> DoublePublicKey<T>::GetSkVch() const
{
    return sk.GetVch();
}
template std::vector<unsigned char> DoublePublicKey<Mcl>::GetSkVch() const;

template <typename T>
std::vector<unsigned char> DoublePublicKey<T>::GetVch() const
{
    auto ret = vk.GetVch();
    auto toAppend = sk.GetVch();

    ret.insert(ret.end(), toAppend.begin(), toAppend.end());

    return ret;
}
template std::vector<unsigned char> DoublePublicKey<Mcl>::GetVch() const;

template <typename T>
PrivateKey<T>::PrivateKey(typename T::Scalar k_)
{
    k.resize(PrivateKey<T>::SIZE);
    std::vector<unsigned char> v = k_.GetVch();
    memcpy(k.data(), &v.front(), k.size());
}
template PrivateKey<Mcl>::PrivateKey(Mcl::Scalar);

template <typename T>
PrivateKey<T>::PrivateKey(CPrivKey k_)
{
    k.resize(PrivateKey<T>::SIZE);
    memcpy(k.data(), &k_.front(), k.size());
}
template PrivateKey<Mcl>::PrivateKey(CPrivKey);

template <typename T>
bool PrivateKey<T>::operator==(const PrivateKey<T>& rhs) const
{
    return k == rhs.k;
}
template bool PrivateKey<Mcl>::operator==(const PrivateKey<Mcl>& rhs) const;

template <typename T>
typename T::Point PrivateKey<T>::GetPoint() const
{
    return T::Point::GetBasePoint() * typename T::Scalar(std::vector<unsigned char>(k.begin(), k.end()));
}
template Mcl::Point PrivateKey<Mcl>::GetPoint() const;

template <typename T>
PublicKey<T> PrivateKey<T>::GetPublicKey() const
{
    return PublicKey<T>(GetPoint());
}
template PublicKey<Mcl> PrivateKey<Mcl>::GetPublicKey() const;

template <typename T>
typename T::Scalar PrivateKey<T>::GetScalar() const
{
    return typename T::Scalar(std::vector<unsigned char>(k.begin(), k.end()));
}
template Mcl::Scalar PrivateKey<Mcl>::GetScalar() const;

template <typename T>
bool PrivateKey<T>::IsValid() const
{
    if (k.size() == 0) return false;
    return GetScalar().IsValid();
}
template bool PrivateKey<Mcl>::IsValid() const;

template <typename T>
void PrivateKey<T>::SetToZero()
{
    k.clear();
}
template void PrivateKey<Mcl>::SetToZero();

} // namespace blsct
