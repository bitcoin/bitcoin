// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEY_H
#define BITCOIN_KEY_H

#include <stdexcept>
#include <vector>

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include "serialize.h"
#include "uint256.h"

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

int extern EC_KEY_regenerate_key(EC_KEY *eckey, BIGNUM *priv_key);
int extern ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig, const unsigned char *msg, int msglen, int recid, int check);

class key_error : public std::runtime_error
{
public:
    explicit key_error(const std::string& str) : std::runtime_error(str) {}
};


// secure_allocator is defined in serialize.h
// CPrivKey is a serialized private key, with all parameters included (279 bytes)
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CPrivKey;
// CSecret is a serialization of just the secret parameter (32 bytes)
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CSecret;

class CKey
{
protected:
    EC_KEY* pkey;
    bool fSet;
    bool fCompressedPubKey;

    void SetCompressedPubKey()
    {
        EC_KEY_set_conv_form(pkey, POINT_CONVERSION_COMPRESSED);
        fCompressedPubKey = true;
    }

public:

    void Reset()
    {
        fCompressedPubKey = false;
        pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (pkey == NULL)
            throw key_error("CKey::CKey() : EC_KEY_new_by_curve_name failed");
        fSet = false;
    }

    CKey()
    {
        Reset();
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

    bool IsCompressed() const
    {
        return fCompressedPubKey;
    }

    void MakeNewKey(bool fCompressed = true)
    {
        if (!EC_KEY_generate_key(pkey))
            throw key_error("CKey::MakeNewKey() : EC_KEY_generate_key failed");
        if (fCompressed)
            SetCompressedPubKey();
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

    bool SetSecret(const CSecret& vchSecret, bool fCompressed = false)
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
        if (fCompressed || fCompressedPubKey)
            SetCompressedPubKey();
        return true;
    }

    CSecret GetSecret(bool &fCompressed) const
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
        fCompressed = fCompressedPubKey;
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
        if (vchPubKey.size() == 33)
            SetCompressedPubKey();
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
        unsigned int nSize = ECDSA_size(pkey);
        vchSig.resize(nSize); // Make sure it is big enough
        if (!ECDSA_sign(0, (unsigned char*)&hash, sizeof(hash), &vchSig[0], &nSize, pkey))
        {
            vchSig.clear();
            return false;
        }
        vchSig.resize(nSize); // Shrink to fit actual size
        return true;
    }

    // create a compact signature (65 bytes), which allows reconstructing the used public key
    // The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
    // The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
    //                  0x1D = second key with even y, 0x1E = second key with odd y
    bool SignCompact(uint256 hash, std::vector<unsigned char>& vchSig)
    {
        bool fOk = false;
        ECDSA_SIG *sig = ECDSA_do_sign((unsigned char*)&hash, sizeof(hash), pkey);
        if (sig==NULL)
            return false;
        vchSig.clear();
        vchSig.resize(65,0);
        int nBitsR = BN_num_bits(sig->r);
        int nBitsS = BN_num_bits(sig->s);
        if (nBitsR <= 256 && nBitsS <= 256)
        {
            int nRecId = -1;
            for (int i=0; i<4; i++)
            {
                CKey keyRec;
                keyRec.fSet = true;
                if (fCompressedPubKey)
                    keyRec.SetCompressedPubKey();
                if (ECDSA_SIG_recover_key_GFp(keyRec.pkey, sig, (unsigned char*)&hash, sizeof(hash), i, 1) == 1)
                    if (keyRec.GetPubKey() == this->GetPubKey())
                    {
                        nRecId = i;
                        break;
                    }
            }

            if (nRecId == -1)
                throw key_error("CKey::SignCompact() : unable to construct recoverable key");

            vchSig[0] = nRecId+27+(fCompressedPubKey ? 4 : 0);
            BN_bn2bin(sig->r,&vchSig[33-(nBitsR+7)/8]);
            BN_bn2bin(sig->s,&vchSig[65-(nBitsS+7)/8]);
            fOk = true;
        }
        ECDSA_SIG_free(sig);
        return fOk;
    }

    // reconstruct public key from a compact signature
    // This is only slightly more CPU intensive than just verifying it.
    // If this function succeeds, the recovered public key is guaranteed to be valid
    // (the signature is a valid signature of the given data for that key)
    bool SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig)
    {
        if (vchSig.size() != 65)
            return false;
        int nV = vchSig[0];
        if (nV<27 || nV>=35)
            return false;
        ECDSA_SIG *sig = ECDSA_SIG_new();
        BN_bin2bn(&vchSig[1],32,sig->r);
        BN_bin2bn(&vchSig[33],32,sig->s);

        EC_KEY_free(pkey);
        pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (nV >= 31)
        {
            SetCompressedPubKey();
            nV -= 4;
        }
        if (ECDSA_SIG_recover_key_GFp(pkey, sig, (unsigned char*)&hash, sizeof(hash), nV - 27, 0) == 1)
        {
            fSet = true;
            ECDSA_SIG_free(sig);
            return true;
        }
        return false;
    }

    bool Verify(uint256 hash, const std::vector<unsigned char>& vchSig)
    {
        // -1 = error, 0 = bad sig, 1 = good
        if (ECDSA_verify(0, (unsigned char*)&hash, sizeof(hash), &vchSig[0], vchSig.size(), pkey) != 1)
            return false;
        return true;
    }

    // Verify a compact signature
    bool VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig)
    {
        CKey key;
        if (!key.SetCompactSignature(hash, vchSig))
            return false;
        if (GetPubKey() != key.GetPubKey())
            return false;
        return true;
    }
};

#endif
