// Copyright (c) 2011 The Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef __CRYPTER_H__
#define __CRYPTER_H__

#include "key.h"

const unsigned int WALLET_CRYPTO_KEY_SIZE = 32;
const unsigned int WALLET_CRYPTO_SALT_SIZE = 8;

class CMasterKey
{
public:
    std::vector<unsigned char> vchCryptedKey;
    std::vector<unsigned char> vchSalt;
    unsigned int nDeriveIterations;
    // 0 = EVP_sha512()
    unsigned int nDerivationMethod;

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchCryptedKey);
        READWRITE(vchSalt);
        READWRITE(nDeriveIterations);
        READWRITE(nDerivationMethod);
    )
    CMasterKey()
    {
        // 25000 rounds is just under 0.1 seconds on a 1.86 GHz Pentium M
        // ie slightly lower than the lowest hardware we need bother supporting
        nDeriveIterations = 25000;
        nDerivationMethod = 0;
    }
};

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;

class CCrypter
{
private:
    unsigned char chKey[WALLET_CRYPTO_KEY_SIZE];
    unsigned char chIV[WALLET_CRYPTO_KEY_SIZE];
    bool fKeySet;

public:
    bool SetKeyFromPassphrase(const std::string &strKeyData, const std::vector<unsigned char>& chSalt, const unsigned int nRounds, const unsigned int nDerivationMethod);
    bool Encrypt(const CKeyingMaterial& vchPlaintext, std::vector<unsigned char> &vchCiphertext);
    bool Decrypt(const std::vector<unsigned char>& vchCiphertext, CKeyingMaterial& vchPlaintext);
    bool SetKey(const CKeyingMaterial& chNewKey, const std::vector<unsigned char>& chNewIV);

    void CleanKey()
    {
        memset(&chKey, 0, sizeof chKey);
        memset(&chIV, 0, sizeof chIV);
        fKeySet = false;
    }

    CCrypter()
    {
        fKeySet = false;
    }

    ~CCrypter()
    {
        CleanKey();
    }
};

    bool EncryptSecret(CKeyingMaterial& vMasterKey, const CSecret &vchPlaintext, const uint256& nIV, std::vector<unsigned char> &vchCiphertext);
    bool DecryptSecret(const CKeyingMaterial& vMasterKey, const std::vector<unsigned char> &vchCiphertext, const uint256& nIV, CSecret &vchPlaintext);

#endif
