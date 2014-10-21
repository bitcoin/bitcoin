// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EC_WRAPPER_H
#define BITCOIN_EC_WRAPPER_H

#include <cstddef>
#include <vector>

#include <openssl/ec.h>

class uint256;

// RAII Wrapper around OpenSSL's EC_KEY
class CECKey {
private:
    EC_KEY *pkey;

public:
    CECKey();
    ~CECKey();

    void GetSecretBytes(unsigned char vch[32]) const;
    void SetSecretBytes(const unsigned char vch[32]);
    int GetPrivKeySize(bool fCompressed);
    int GetPrivKey(unsigned char* privkey, bool fCompressed);
    bool SetPrivKey(const unsigned char* privkey, size_t size, bool fSkipCheck=false);
    void GetPubKey(std::vector<unsigned char>& pubkey, bool fCompressed);
    bool SetPubKey(const unsigned char* pubkey, size_t size);
    bool Sign(const uint256 &hash, std::vector<unsigned char>& vchSig, bool lowS);
    bool Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig);
    bool SignCompact(const uint256 &hash, unsigned char *p64, int &rec);

    // reconstruct public key from a compact signature
    // This is only slightly more CPU intensive than just verifying it.
    // If this function succeeds, the recovered public key is guaranteed to be valid
    // (the signature is a valid signature of the given data for that key)
    bool Recover(const uint256 &hash, const unsigned char *p64, int rec);

    static bool TweakSecret(unsigned char vchSecretOut[32], const unsigned char vchSecretIn[32], const unsigned char vchTweak[32]);
    bool TweakPublic(const unsigned char vchTweak[32]);
    static bool SanityCheck();
};

#endif
