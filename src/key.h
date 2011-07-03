// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEY_H
#define BITCOIN_KEY_H

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

// secp160k1
// const unsigned int PRIVATE_KEY_SIZE = 192;
// const unsigned int PUBLIC_KEY_SIZE  = 41;
// const unsigned int SIGNATURE_SIZE   = 48;
//
// secp192k1
// const unsigned int PRIVATE_KEY_SIZE = 222;
// const unsigned int PUBLIC_KEY_SIZE  = 49;
// const unsigned int SIGNATURE_SIZE   = 57;
//
// secp224k1
// const unsigned int PRIVATE_KEY_SIZE = 250;
// const unsigned int PUBLIC_KEY_SIZE  = 57;
// const unsigned int SIGNATURE_SIZE   = 66;
//
// secp256k1:
// const unsigned int PRIVATE_KEY_SIZE = 279;
// const unsigned int PUBLIC_KEY_SIZE  = 65;
// const unsigned int SIGNATURE_SIZE   = 72;
//
// see www.keylength.com
// script supports up to 75 for single byte push

int static inline EC_KEY_regenerate_key(EC_KEY *eckey, BIGNUM *priv_key)
{
    int ok = 0;
    BN_CTX *ctx = NULL;
    EC_POINT *pub_key = NULL;

    if (!eckey) return 0;

    const EC_GROUP *group = EC_KEY_get0_group(eckey);

    if ((ctx = BN_CTX_new()) == NULL)
        goto err;

    pub_key = EC_POINT_new(group);

    if (pub_key == NULL)
        goto err;

    if (!EC_POINT_mul(group, pub_key, priv_key, NULL, NULL, ctx))
        goto err;

    EC_KEY_set_private_key(eckey,priv_key);
    EC_KEY_set_public_key(eckey,pub_key);

    ok = 1;

err:

    if (pub_key)
        EC_POINT_free(pub_key);
    if (ctx != NULL)
        BN_CTX_free(ctx);

    return(ok);
}


class key_error : public std::runtime_error
{
public:
    explicit key_error(const std::string& str) : std::runtime_error(str) {}
};


// secure_allocator is defined in serialize.h
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CPrivKey;
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CSecret;

class CKey
{
protected:
    EC_KEY* pkey;
    bool fSet;

public:
    CKey()
    {
        pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (pkey == NULL)
            throw key_error("CKey::CKey() : EC_KEY_new_by_curve_name failed");
        fSet = false;
    }

    CKey(const CKey& b)
    {
        pkey = EC_KEY_dup(b.pkey);
        if (pkey == NULL)
            throw key_error("CKey::CKey(const CKey&) : EC_KEY_dup failed");
        fSet = b.fSet;
    }

    CKey& operator=(const CKey& b)
    {
        if (!EC_KEY_copy(pkey, b.pkey))
            throw key_error("CKey::operator=(const CKey&) : EC_KEY_copy failed");
        fSet = b.fSet;
        return (*this);
    }

    ~CKey()
    {
        EC_KEY_free(pkey);
    }

    bool IsNull() const
    {
        return !fSet;
    }

    void MakeNewKey()
    {
        if (!EC_KEY_generate_key(pkey))
            throw key_error("CKey::MakeNewKey() : EC_KEY_generate_key failed");
        fSet = true;
    }

    bool SetPrivKey(const CPrivKey& vchPrivKey)
    {
        const unsigned char* pbegin = &vchPrivKey[0];
        if (!d2i_ECPrivateKey(&pkey, &pbegin, vchPrivKey.size()))
            return false;
        fSet = true;
        return true;
    }

    bool SetSecret(const CSecret& vchSecret)
    {
        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (pkey == NULL)
            throw key_error("CKey::SetSecret() : EC_KEY_new_by_curve_name failed");
        if (vchSecret.size() != 32)
            throw key_error("CKey::SetSecret() : secret must be 32 bytes");
        BIGNUM *bn = BN_bin2bn(&vchSecret[0],32,BN_new());
        if (bn == NULL) 
            throw key_error("CKey::SetSecret() : BN_bin2bn failed");
        if (!EC_KEY_regenerate_key(pkey,bn))
            throw key_error("CKey::SetSecret() : EC_KEY_regenerate_key failed");
        BN_clear_free(bn);
        fSet = true;
        return true;
    }

    CSecret GetSecret() const
    {
        CSecret vchRet;
        vchRet.resize(32);
        const BIGNUM *bn = EC_KEY_get0_private_key(pkey);
        int nBytes = BN_num_bytes(bn);
        if (bn == NULL)
            throw key_error("CKey::GetSecret() : EC_KEY_get0_private_key failed");
        int n=BN_bn2bin(bn,&vchRet[32 - nBytes]);
        if (n != nBytes) 
            throw key_error("CKey::GetSecret(): BN_bn2bin failed");
        return vchRet;
    }

    CPrivKey GetPrivKey() const
    {
        unsigned int nSize = i2d_ECPrivateKey(pkey, NULL);
        if (!nSize)
            throw key_error("CKey::GetPrivKey() : i2d_ECPrivateKey failed");
        CPrivKey vchPrivKey(nSize, 0);
        unsigned char* pbegin = &vchPrivKey[0];
        if (i2d_ECPrivateKey(pkey, &pbegin) != nSize)
            throw key_error("CKey::GetPrivKey() : i2d_ECPrivateKey returned unexpected size");
        return vchPrivKey;
    }

    bool SetPubKey(const std::vector<unsigned char>& vchPubKey)
    {
        const unsigned char* pbegin = &vchPubKey[0];
        if (!o2i_ECPublicKey(&pkey, &pbegin, vchPubKey.size()))
            return false;
        fSet = true;
        return true;
    }

    std::vector<unsigned char> GetPubKey() const
    {
        unsigned int nSize = i2o_ECPublicKey(pkey, NULL);
        if (!nSize)
            throw key_error("CKey::GetPubKey() : i2o_ECPublicKey failed");
        std::vector<unsigned char> vchPubKey(nSize, 0);
        unsigned char* pbegin = &vchPubKey[0];
        if (i2o_ECPublicKey(pkey, &pbegin) != nSize)
            throw key_error("CKey::GetPubKey() : i2o_ECPublicKey returned unexpected size");
        return vchPubKey;
    }

    bool Sign(uint256 hash, std::vector<unsigned char>& vchSig)
    {
        vchSig.clear();
        unsigned char pchSig[10000];
        unsigned int nSize = 0;
        if (!ECDSA_sign(0, (unsigned char*)&hash, sizeof(hash), pchSig, &nSize, pkey))
            return false;
        vchSig.resize(nSize);
        memcpy(&vchSig[0], pchSig, nSize);
        return true;
    }

    bool Verify(uint256 hash, const std::vector<unsigned char>& vchSig)
    {
        // -1 = error, 0 = bad sig, 1 = good
        if (ECDSA_verify(0, (unsigned char*)&hash, sizeof(hash), &vchSig[0], vchSig.size(), pkey) != 1)
            return false;
        return true;
    }
};

#endif
