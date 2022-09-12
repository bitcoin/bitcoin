// Copyright (c) 2020 The Navcoin developers
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

#include <bls.hpp>
#include <blsct/scalar.h>
#include <hash.h>
#include <hdkeys.hpp>
#include <key.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <version.h>

static const std::string subAddressHeader = "SubAddress\0";

class blsctDoublePublicKey
{
public:
    blsctDoublePublicKey() { vk.clear(); sk.clear(); }
    blsctDoublePublicKey(const bls::G1Element& vk_, const bls::G1Element& sk_) : vk(vk_.Serialize()), sk(sk_.Serialize()) {}
    blsctDoublePublicKey(const std::vector<uint8_t>& vk_, const std::vector<uint8_t>& sk_) : vk(vk_), sk(sk_) {}

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(vk, nType, nVersion) + ::GetSerializeSize(sk, nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, vk, nType, nVersion);
        ::Serialize(s, sk, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        ::Unserialize(s, vk, nType, nVersion);
        ::Unserialize(s, sk, nType, nVersion);
    }

    uint256 GetHash() const {
        CHashWriter ss(SER_GETHASH, 0);
        ss << vk;
        ss << sk;
        return ss.GetHash();
    }

    CKeyID GetID() const
    {
        return CKeyID(Hash160(sk.data(), sk.data() + sk.size()));
    }

    bool GetViewKey(bls::G1Element &ret) const {
        try
        {
            ret = bls::G1Element::FromBytes(&vk.front());
        }
        catch(...)
        {
            return false;
        }

        return true;
    }

    bool GetSpendKey(bls::G1Element &ret) const {
        try
        {
            ret = bls::G1Element::FromBytes(&sk.front());
        }
        catch(...)
        {
            return false;
        }

        return true;
    }

    bool operator<(const blsctDoublePublicKey& rhs) const {
        bls::G1Element l, r;
        try
        {
            l = bls::G1Element::FromBytes(&vk.front());
            r = bls::G1Element::FromBytes(&(rhs.vk).front());
        }
        catch(...)
        {
            return false;
        }
        return g1_cmp(l.p, r.p);
    }

    bool operator==(const blsctDoublePublicKey& rhs) const {
        return vk==rhs.vk && sk==rhs.sk;
    }

    bool IsValid() const {
        return vk.size() > 0 && sk.size() > 0;
    }

    std::vector<uint8_t> GetVkVch() const {
        return vk;
    }

    std::vector<uint8_t> GetSkVch() const {
        return sk;
    }

protected:
    std::vector<uint8_t> vk;
    std::vector<uint8_t> sk;
};

class blsctPublicKey
{
public:
    blsctPublicKey() { vk.clear(); }
    blsctPublicKey(const bls::G1Element& vk_) : vk(vk_.Serialize()) {}
    blsctPublicKey(const std::vector<uint8_t>& vk_) : vk(vk_) {}

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(vk, nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, vk, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        ::Unserialize(s, vk, nType, nVersion);
    }

    uint256 GetHash() const {
        CHashWriter ss(SER_GETHASH, 0);
        ss << vk;
        return ss.GetHash();
    }

    CKeyID GetID() const
    {
        return CKeyID(Hash160(vk.data(), vk.data() + vk.size()));
    }

    bool GetG1Element(bls::G1Element& ret) const {
        try
        {
            ret = bls::G1Element::FromBytes(&vk.front());
        }
        catch(...)
        {
            return false;
        }
        return true;
    }

    bool operator<(const blsctPublicKey& rhs) const {
        bls::G1Element l, r;
        try
        {
            l = bls::G1Element::FromBytes(&vk.front());
            r = bls::G1Element::FromBytes(&(rhs.vk).front());
        }
        catch(...)
        {
            return false;
        }
        return g1_cmp(l.p, r.p);
    }

    bool operator==(const blsctPublicKey& rhs) const {
        return vk==rhs.vk;
    }

    bool IsValid() const {
        return vk.size() > 0;
    }

    std::vector<uint8_t> GetVch() const {
        return vk;
    }

protected:
    std::vector<uint8_t> vk;
};

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

    bls::G1Element GetG1Element() const {
        try {
            return bls::PrivateKey::FromBytes(&k.front()).GetG1Element();
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
        return bls::HDKeys::DeriveChildSk(buf, i).GetG1Element();
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

#endif // KEY_H
