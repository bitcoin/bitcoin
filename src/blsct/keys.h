// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KEY_H
#define KEY_H

#include <blsct/arith/g1point.h>
#include <blsct/arith/scalar.h>
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

class PublicKey
{
private:
    std::vector<unsigned char> data;

public:
    static constexpr size_t SIZE = 48;

    PublicKey() { data.clear(); }
    PublicKey(const G1Point& pk) : data(pk.GetVch()) {}
    PublicKey(const std::vector<unsigned char>& pk) : data(pk) {}

    SERIALIZE_METHODS(PublicKey, obj) { READWRITE(obj.data); }

    static PublicKey Aggregate(std::vector<PublicKey> vPk);

    uint256 GetHash() const;
    CKeyID GetID() const;

    std::string ToString() const;

    bool operator==(const PublicKey& rhs) const;

    bool IsValid() const;

    bool GetG1Point(G1Point& ret) const;
    std::vector<unsigned char> GetVch() const;
};

class DoublePublicKey
{
private:
    PublicKey vk;
    PublicKey sk;

public:
    static constexpr size_t SIZE = 48 * 2;

    DoublePublicKey() {}
    DoublePublicKey(const G1Point& vk_, const G1Point& sk_) : vk(vk_.GetVch()), sk(sk_.GetVch()) {}
    DoublePublicKey(const std::vector<unsigned char>& vk_, const std::vector<unsigned char>& sk_) : vk(vk_), sk(sk_) {}

    SERIALIZE_METHODS(DoublePublicKey, obj) { READWRITE(obj.vk.GetVch(), obj.sk.GetVch()); }

    uint256 GetHash() const;
    CKeyID GetID() const;

    bool GetViewKey(G1Point& ret) const;
    bool GetSpendKey(G1Point& ret) const;

    bool operator==(const DoublePublicKey& rhs) const;

    bool IsValid() const;

    std::vector<unsigned char> GetVkVch() const;
    std::vector<unsigned char> GetSkVch() const;
    std::vector<unsigned char> GetVch() const;
};

class PrivateKey
{
private:
    CPrivKey k;

public:
    static constexpr size_t SIZE = 32;

    PrivateKey() { k.clear(); }

    PrivateKey(Scalar k_)
    {
        k.resize(PrivateKey::SIZE);
        std::vector<unsigned char> v = k_.GetVch();
        memcpy(k.data(), &v.front(), k.size());
    }

    PrivateKey(CPrivKey k_)
    {
        k.resize(PrivateKey::SIZE);
        memcpy(k.data(), &k_.front(), k.size());
    }

    SERIALIZE_METHODS(PrivateKey, obj) { READWRITE(std::vector<unsigned char>(obj.k.begin(), obj.k.end())); }

    bool operator==(const PrivateKey& rhs) const;

    G1Point GetG1Point() const;
    PublicKey GetPublicKey() const;
    Scalar GetScalar() const;

    bool IsValid() const;

    void SetToZero();

    friend class CCryptoKeyStore;
    friend class CBasicKeyStore;
};

} // namespace blsct

#endif // KEY_H
