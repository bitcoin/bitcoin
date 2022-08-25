// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KEY_H
#define KEY_H

#ifdef _WIN32
/* Avoid redefinition warning. */
#undef ERROR
#undef WSIZE
#undef DOUBLE
#endif

#include <blsct/arith/scalar.h>
#include <blsct/arith/g1point.h>
#include <hash.h>
#include <key.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <version.h>

namespace blsct
{
static const std::string subAddressHeader = "SubAddress\0";

class PublicKey
{
public:
    PublicKey() { data.clear(); }
    PublicKey(const G1Point& pk) : data(pk.GetVch()) {}
    PublicKey(const std::vector<uint8_t>& pk) : data(pk) {}

    SERIALIZE_METHODS(PublicKey, obj) { READWRITE(obj.data); }

    uint256 GetHash() const;
    CKeyID GetID() const;

    bool operator==(const PublicKey& rhs) const;

    bool IsValid() const;

    bool GetG1Point(G1Point& ret) const;
    std::vector<uint8_t> GetVch() const;

protected:
    std::vector<uint8_t> data;
};

class DoublePublicKey
{
public:
    DoublePublicKey() {}
    DoublePublicKey(const G1Point& vk_, const G1Point& sk_) : vk(vk_.GetVch()), sk(sk_.GetVch()) {}
    DoublePublicKey(const std::vector<uint8_t>& vk_, const std::vector<uint8_t>& sk_) : vk(vk_), sk(sk_) {}

    SERIALIZE_METHODS(DoublePublicKey, obj) { READWRITE(obj.vk.GetVch(), obj.sk.GetVch()); }

    uint256 GetHash() const;
    CKeyID GetID() const;

    bool GetViewKey(G1Point &ret) const;
    bool GetSpendKey(G1Point &ret) const;

    bool operator==(const DoublePublicKey& rhs) const;

    bool IsValid() const;

    std::vector<uint8_t> GetVkVch() const;
    std::vector<uint8_t> GetSkVch() const;

protected:
    PublicKey vk;
    PublicKey sk;
};

/*
class blsctKey
{
private:
    CPrivKey k;

public:
    blsctKey() { k.clear(); }

    blsctKey(bls::PrivateKey k_) {
        k.resize(bls::PrivateKey::PRIVATE_KEY_SIZE);
        std::vector<uint8_t> v = k_.Serialize();
        memcpy(k.data(), &v.front(), k.size());
    }

    blsctKey(CPrivKey k_) {
        k.resize(bls::PrivateKey::PRIVATE_KEY_SIZE);
        memcpy(k.data(), &k_.front(), k.size());
    }

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(k, nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, k, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        ::Unserialize(s, k, nType, nVersion);
    }

    bool operator<(const blsctKey& rhs) const {
        try {
            Scalar l, r;
            l = bls::PrivateKey::FromBytes(&k.front());
            r = bls::PrivateKey::FromBytes(&(rhs.k).front());
            return bn_cmp(l.bn, r.bn);
        } catch (...) {
            return false;
        }
    }

    bool operator==(const blsctKey& rhs) const {
        return k==rhs.k;
    }

    bls::G1Element GetG1Point() const {
        try {
            return bls::PrivateKey::FromBytes(&k.front()).GetG1Point();
        } catch(...) {
            return bls::G1Element();
        }
    }

    bls::PrivateKey GetKey() const {
        try {
            return bls::PrivateKey::FromBytes(&k.front());
        } catch(...) {
            return bls::PrivateKey::FromBN(Scalar::Rand().bn);
        }
    }

    Scalar GetScalar() const {
        return Scalar(GetKey());
    }

    bls::PrivateKey PrivateChild(uint32_t i) const {
        try {
            bls::PrivateKey buf = bls::PrivateKey::FromBytes(&k.front());
            return bls::HDKeys::DeriveChildSk(buf, i);
        } catch(...) {
            return bls::PrivateKey::FromBN(Scalar::Rand().bn);
        }
    }

    bls::PrivateKey PrivateChildHash(uint256 h) const {
        try {
            bls::PrivateKey ret = bls::PrivateKey::FromBytes(&k.front());
            for (auto i = 0; i < 8; i++)
            {
                const uint8_t* pos = h.begin() + i*4;
                uint32_t index = (uint8_t)(pos[0]) << 24 |
                                 (uint8_t)(pos[1]) << 16 |
                                 (uint8_t)(pos[2]) << 8  |
                                 (uint8_t)(pos[3]);
                ret = bls::HDKeys::DeriveChildSk(ret, index);
            }
            return ret;
        } catch(...) {
            return  bls::PrivateKey::FromBN(Scalar::Rand().bn);
        }
    }

    bls::G1Element PublicChild(uint32_t i) const {
        bls::PrivateKey buf = bls::PrivateKey::FromBytes(&k.front());
        return bls::HDKeys::DeriveChildSk(buf, i).GetG1Point();
    }

    bool IsValid() const {
        return k.size() > 0;
    }

    void SetToZero() {
        k.clear();
    }

    friend class CCryptoKeyStore;
    friend class CBasicKeyStore;
};
*/
}

#endif // KEY_H
