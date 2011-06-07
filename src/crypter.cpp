#include <openssl/aes.h>
#include <openssl/evp.h>
#include <vector>
#include <string>
#ifdef __WXMSW__
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "crypter.h"
#include "main.h"
#include "util.h"

bool CCrypter::SetKey(const std::string &strKeyData)
{
    std::vector<unsigned char> vchKeyData(strKeyData.size());
    unsigned char chNotIV[32];

    // Try to keep the keydata out of swap (and be a bit over-careful to keep the IV that we don't even use out of swap)
    // Note that this does nothing about suspend-to-disk (which will put all our key data on disk)
    // Note as well that at no point in this program is any attempt made to prevent stealing of keys by reading the memory of the running process.  
    MLOCK(vchKeyData[0], vchKeyData.size());
    MLOCK(chNotIV[0], sizeof chNotIV);
    MLOCK(chKey[0], sizeof chKey);

    memcpy(&vchKeyData[0], &strKeyData[0], strKeyData.size());

    int i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), (unsigned char *)"bitcoin is fun! and I prefer much longer salts, though I don't think they offer any real advantage",
                           (unsigned char *)&vchKeyData[0], vchKeyData.size(), 1000, chKey, chNotIV);

    std::fill(vchKeyData.begin(), vchKeyData.end(), '\0');
    memset(&chNotIV, 0, sizeof chNotIV);

    if (i != 32)
    {
        memset(&chKey, 0, sizeof chKey);
        return false;
    }

    fCorrectKey = false;
    fKeySet = true;
    return true;
}

bool CCrypter::Encrypt(const std::vector<unsigned char> &vchPlaintext, const unsigned char chIV[32], std::vector<unsigned char> &vchCiphertext)
{
    if (!fKeySet)
        return false;

    // max ciphertext len for a n bytes of plaintext is
    // n + AES_BLOCK_SIZE - 1 bytes
    int nLen = vchPlaintext.size();
    int nCLen = nLen + AES_BLOCK_SIZE, nFLen = 0;
    vchCiphertext = std::vector<unsigned char> (nCLen);

    EVP_CIPHER_CTX ctx;

    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, chKey, chIV);

    EVP_EncryptUpdate(&ctx, &vchCiphertext[0], &nCLen, &vchPlaintext[0], nLen);
    EVP_EncryptFinal_ex(&ctx, (&vchCiphertext[0])+nCLen, &nFLen);

    EVP_CIPHER_CTX_cleanup(&ctx);

    vchCiphertext.resize(nCLen + nFLen);
    return true;
}

bool CCrypter::Decrypt(const std::vector<unsigned char> &vchCiphertext, const unsigned char chIV[32], std::vector<unsigned char> &vchPlaintext)
{
    if (!fKeySet)
        return false;

    // plaintext will always be equal to or lesser than length of ciphertext
    int nLen = vchCiphertext.size();
    int nPLen = nLen, nFLen = 0;

    vchPlaintext = std::vector<unsigned char> (nPLen);
    MLOCK(vchPlaintext[0], vchPlaintext.size());

    EVP_CIPHER_CTX ctx;

    EVP_CIPHER_CTX_init(&ctx);
    EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, chKey, chIV);

    EVP_DecryptUpdate(&ctx, &vchPlaintext[0], &nPLen, &vchCiphertext[0], nLen);
    EVP_DecryptFinal_ex(&ctx, (&vchPlaintext[0])+nPLen, &nFLen);

    EVP_CIPHER_CTX_cleanup(&ctx);

    vchPlaintext.resize(nPLen + nFLen);
    return true;
}

bool CheckKeyOnPrivKey(PubToPrivKeyMap::iterator &pairFirstKey)
{
    uint256 hashPubKey = Hash((*pairFirstKey).first.begin(), (*pairFirstKey).first.end());
    unsigned char chIV[32];
    memcpy(&chIV, &hashPubKey, 32);

    std::vector<unsigned char> vchPlaintext;
    std::vector<unsigned char> vchCiphertext;
    vchCiphertext.resize((*pairFirstKey).second.size());
    memcpy(&vchCiphertext[0], &(*pairFirstKey).second[0], vchCiphertext.size());

    if (!cWalletCrypter.Decrypt(vchCiphertext, chIV, vchPlaintext)) //mlock()s vchPlaintext for us
        return false;

    CPrivKey vchPrivKey;
    vchPrivKey.resize(vchPlaintext.size());
    MLOCK(vchPrivKey[0], vchPrivKey.size());

    memcpy(&vchPrivKey[0], &vchPlaintext[0], vchPlaintext.size());
    std::fill(vchPlaintext.begin(), vchPlaintext.end(), '\0');

    CKey key;
    if (!key.SetPrivKey(vchPrivKey))
    {
        std::fill(vchPrivKey.begin(), vchPrivKey.end(), '\0');
        return false;
    }
    std::vector<unsigned char> vchDerivedPubKey = key.GetPubKey();
    if (vchDerivedPubKey.size() < 1 || vchDerivedPubKey != (*pairFirstKey).first)
    {
        std::fill(vchPrivKey.begin(), vchPrivKey.end(), '\0');
        return false;
    }

    std::fill(vchPrivKey.begin(), vchPrivKey.end(), '\0');
    return true;
}

bool CCrypter::CheckKey(const bool fVerifyAllAddresses)
{
    if (fCorrectKeyForAllKeys)
        return true;
    if (fCorrectKey && !fVerifyAllAddresses)
        return true;

    if (mapKeys.size() == 0)
    {
        fCorrectKey = true;
        return true;
    }

    PubToPrivKeyMap::iterator pairFirstKey = mapKeys.begin();

    if (!fVerifyAllAddresses)
    {
        fCorrectKey = CheckKeyOnPrivKey(pairFirstKey);
        return fCorrectKey;
    }
    else
    {
        fCorrectKeyForAllKeys = true;
        fCorrectKey = true;
        for (; pairFirstKey != mapKeys.end(); pairFirstKey++)
        {
            fCorrectKeyForAllKeys = CheckKeyOnPrivKey(pairFirstKey);
            fCorrectKey = fCorrectKeyForAllKeys;
            if (!fCorrectKey)
                return false;
        }
    }

    return true;
}

void CCrypter::CleanKey()
{
    memset(&chKey, 0, sizeof chKey);
    fCorrectKey = false;
    fKeySet = false;
}
