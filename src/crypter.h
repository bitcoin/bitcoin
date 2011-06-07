#ifndef __CRYPTER_H__
#define __CRYPTER_H__

class CCrypter
{
private:
    unsigned char chKey[32];
    bool fCorrectKey;
    bool fCorrectKeyForAllKeys;

public:
    bool fKeySet;
    bool SetKey(const std::string &strKeyData);
    bool Encrypt(const std::vector<unsigned char> &vchPlaintext, const unsigned char chIV[32], std::vector<unsigned char> &vchCiphertext);
    bool Decrypt(const std::vector<unsigned char> &vchCiphertext, const unsigned char chIV[32], std::vector<unsigned char> &vchPlaintext);

    // Only call after wallet has been loaded
    bool CheckKey(const bool fVerifyAllAddresses = false);

    void CleanKey();
    CCrypter()
    {
        fCorrectKey = false;
        fKeySet = false;
    }

    ~CCrypter()
    {
        CleanKey();
    }
};

#endif /* __CRYPTER_H__ */
