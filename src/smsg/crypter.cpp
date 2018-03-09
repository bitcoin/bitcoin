// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <smsg/crypter.h>
#include <crypto/aes.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif


bool SecMsgCrypter::SetKey(const std::vector<uint8_t> &vchNewKey, const uint8_t *chNewIV)
{
    if (vchNewKey.size() != SMSG_CRYPTO_KEY_SIZE)
        return false;

    return SetKey(&vchNewKey[0], chNewIV);
};

bool SecMsgCrypter::SetKey(const uint8_t *chNewKey, const uint8_t *chNewIV)
{
    // For EVP_aes_256_cbc() key must be 256 bit, iv must be 128 bit.
    memcpy(vchKey.data(), chNewKey, vchKey.size());
    memcpy(vchIV.data(), chNewIV, vchIV.size());

    fKeySet = true;
    return true;
};

bool SecMsgCrypter::Encrypt(const uint8_t *chPlaintext, uint32_t nPlain, std::vector<uint8_t> &vchCiphertext)
{
    if (!fKeySet)
        return false;

#ifdef ENABLE_WALLET
    // Max ciphertext len for a n bytes of plaintext is n + AES_BLOCKSIZE - 1 bytes
    vchCiphertext = std::vector<uint8_t> (nPlain + AES_BLOCKSIZE);

    AES256CBCEncrypt aes_en(vchKey.data(), vchIV.data(), true);
    int nCLen = aes_en.Encrypt(chPlaintext, nPlain, &vchCiphertext[0]);

    if (nCLen < (int)nPlain)
        return false;
    vchCiphertext.resize(nCLen);
    return true;
#endif
    return false;
};

bool SecMsgCrypter::Decrypt(const uint8_t *chCiphertext, uint32_t nCipher, std::vector<uint8_t> &vchPlaintext)
{
    if (!fKeySet)
        return false;

#ifdef ENABLE_WALLET
    // plaintext will always be equal to or lesser than length of ciphertext
    vchPlaintext.resize(nCipher);

    AES256CBCDecrypt aes_de(vchKey.data(), vchIV.data(), true);
    int nPLen = aes_de.Decrypt(chCiphertext, nCipher, &vchPlaintext[0]);

    if (nPLen < 0)
        return false;

    vchPlaintext.resize(nPLen);
    return true;
#endif
    return false;
};

