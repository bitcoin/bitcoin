// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KEY_H
#define KEY_H

#include <hash.h>
#include <key.h>
#include <serialize.h>
#include <streams.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <version.h>

namespace blsct {
static const std::string subAddressHeader = "SubAddress\0";

template <typename T>
class PublicKey
{
private:
    using Point = typename T::Point;

    std::vector<unsigned char> data;

public:
    static constexpr size_t SIZE = 48;

    PublicKey() { data.clear(); }
    PublicKey(const Point& pk) : data(pk.GetVch()) {}
    PublicKey(const std::vector<unsigned char>& pk) : data(pk) {}

    SERIALIZE_METHODS(PublicKey<T>, obj) { READWRITE(obj.data); }

    static PublicKey<T> Aggregate(std::vector<PublicKey<T>> vPk);

    uint256 GetHash() const;
    CKeyID GetID() const;

    std::string ToString() const;

    bool operator==(const PublicKey<T>& rhs) const;

    bool IsValid() const;

    bool GetG1Point(Point& ret) const;
    std::vector<unsigned char> GetVch() const;
};

template <typename T>
class DoublePublicKey
{
private:
    using Point = typename T::Point;

    PublicKey<T> vk;
    PublicKey<T> sk;

public:
    static constexpr size_t SIZE = 48 * 2;

    DoublePublicKey() {}
    DoublePublicKey(const Point& vk_, const Point& sk_) : vk(vk_.GetVch()), sk(sk_.GetVch()) {}
    DoublePublicKey(const std::vector<unsigned char>& vk_, const std::vector<unsigned char>& sk_) : vk(vk_), sk(sk_) {}

    SERIALIZE_METHODS(DoublePublicKey<T>, obj) { READWRITE(obj.vk.GetVch(), obj.sk.GetVch()); }

    uint256 GetHash() const;
    CKeyID GetID() const;

    bool GetViewKey(Point& ret) const;
    bool GetSpendKey(Point& ret) const;

    bool operator==(const DoublePublicKey<T>& rhs) const;

    bool IsValid() const;

    std::vector<unsigned char> GetVkVch() const;
    std::vector<unsigned char> GetSkVch() const;
    std::vector<unsigned char> GetVch() const;
};

template <typename T>
class PrivateKey
{
private:
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;

    CPrivKey k;

public:
    static constexpr size_t SIZE = 32;

    PrivateKey() { k.clear(); }
    PrivateKey(Scalar k_);
    PrivateKey(CPrivKey k_);

    SERIALIZE_METHODS(PrivateKey<T>, obj) { READWRITE(std::vector<unsigned char>(obj.k.begin(), obj.k.end())); }

    bool operator==(const PrivateKey<T>& rhs) const;

    Point GetPoint() const;
    PublicKey<T> GetPublicKey() const;
    Scalar GetScalar() const;
    bool IsValid() const;
    void SetToZero();

    friend class CCryptoKeyStore;
    friend class CBasicKeyStore;
};

} // namespace blsct

#endif // KEY_H
