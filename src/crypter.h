#ifndef __CRYPTER_H__
#define __CRYPTER_H__

class CCrypter
{
protected:
    unsigned char chKey[32];
    bool fKeySet;

public:
    bool SetKey(const string &strKeyData)
    {
        vector<unsigned char> vchKeyData(strKeyData.size());
        unsigned char chNotIV[32];

        // try to keep the keydata out of swap (and be a bit over-careful to keep the IV that we don't even use out of swap)
        // Note that this does nothing about suspend-to-disk
#ifdef __WXMSW__
        VirtualLock(&vchKeyData, vchKeyData.size());
        VirtualLock(&chNotIV, sizeof chNotIV);
        VirtualLock(&chKey, sizeof chKey);
#else
        mlock(&vchKeyData, vchKeyData.size());
        mlock(&chNotIV, sizeof chNotIV);
        mlock(&chKey, sizeof chKey);
#endif

        memcpy(&vchKeyData[0], &strKeyData[0], strKeyData.size());

        int i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), (unsigned char *)"bitcoin is fun! and I prefer much longer salts, though I don't think they offer any real advantage",
                               (unsigned char *)&vchKeyData[0], vchKeyData.size(), 1000, chKey, chNotIV);

        fill(vchKeyData.begin(), vchKeyData.end(), '\0');
        memset(&chNotIV, 0, sizeof chNotIV);

        if (i != 32)
            return false;

        fKeySet = true;
        return true;
    }

    bool Encrypt(const vector<unsigned char> &vchPlaintext, const unsigned char chIV[32], vector<unsigned char> &vchCiphertext)
    {
        if (!fKeySet)
            return false;

        // max ciphertext len for a n bytes of plaintext is
        // n + AES_BLOCK_SIZE - 1 bytes
        int nLen = vchPlaintext.size();
        int nCLen = nLen + AES_BLOCK_SIZE, nFLen = 0;
        vchCiphertext = vector<unsigned char> (nCLen);

        EVP_CIPHER_CTX ctx;

        EVP_CIPHER_CTX_init(&ctx);
        EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, chKey, chIV);

        EVP_EncryptUpdate(&ctx, &vchCiphertext[0], &nCLen, &vchPlaintext[0], nLen);
        EVP_EncryptFinal_ex(&ctx, (&vchCiphertext[0])+nCLen, &nFLen);

        EVP_CIPHER_CTX_cleanup(&ctx);

        vchCiphertext.resize(nCLen + nFLen);
        return true;
    }

    bool Decrypt(const vector<unsigned char> vchCiphertext, const unsigned char chIV[32], vector<unsigned char>& vchPlaintext)
    {
        if (!fKeySet)
            return false;

        // plaintext will always be equal to or lesser than length of ciphertext
        int nLen = vchCiphertext.size();
        int nPLen = nLen, nFLen = 0;
        vchPlaintext = vector<unsigned char> (nPLen);

        EVP_CIPHER_CTX ctx;

        EVP_CIPHER_CTX_init(&ctx);
        EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, chKey, chIV);

        EVP_DecryptUpdate(&ctx, &vchPlaintext[0], &nPLen, &vchCiphertext[0], nLen);
        EVP_DecryptFinal_ex(&ctx, (&vchPlaintext[0])+nPLen, &nFLen);

        EVP_CIPHER_CTX_cleanup(&ctx);

        vchPlaintext.resize(nPLen + nFLen);
        return true;
    }

    CCrypter()
    {
        fKeySet = false;
    }

    ~CCrypter()
    {
        memset(&chKey, 0, sizeof chKey);
    }
};

#endif /* __CRYPTER_H__ */
