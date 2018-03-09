// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_SMSG_CRYPTER_H
#define PARTICL_SMSG_CRYPTER_H

#include <support/allocators/secure.h>
#include <vector>
#include <string.h>

const unsigned int SMSG_CRYPTO_KEY_SIZE = 32;
const unsigned int SMSG_CRYPTO_SALT_SIZE = 8;
const unsigned int SMSG_CRYPTO_IV_SIZE = 16;

class SecMsgCrypter
{
private:
    std::vector<unsigned char, secure_allocator<unsigned char>> vchKey;
    std::vector<unsigned char, secure_allocator<unsigned char>> vchIV;
    bool fKeySet;
public:

    SecMsgCrypter()
    {
        fKeySet = false;
        vchKey.resize(SMSG_CRYPTO_KEY_SIZE);
        vchIV.resize(SMSG_CRYPTO_IV_SIZE);
    };

    ~SecMsgCrypter()
    {
        CleanKey();
    };

    void CleanKey()
    {
        memory_cleanse(vchKey.data(), vchKey.size());
        memory_cleanse(vchIV.data(), vchIV.size());
        fKeySet = false;
    }

    bool SetKey(const std::vector<uint8_t> &vchNewKey, const uint8_t *chNewIV);
    bool SetKey(const uint8_t *chNewKey, const uint8_t *chNewIV);
    bool Encrypt(const uint8_t *chPlaintext,  uint32_t nPlain,  std::vector<uint8_t> &vchCiphertext);
    bool Decrypt(const uint8_t *chCiphertext, uint32_t nCipher, std::vector<uint8_t> &vchPlaintext);
};

#endif // PARTICL_SMSG_CRYPTER_H
