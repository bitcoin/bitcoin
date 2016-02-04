// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Copyright (c) 2014-2015 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef CEC_KEY_H
#define CEC_KEY_H

#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/rand.h>
#include <openssl/obj_mac.h>

#include "key.h"

// RAII Wrapper around OpenSSL's EC_KEY
class CECKey {
private:
    EC_KEY *pkey;

public:
    CECKey() {
        pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        assert(pkey != NULL);
    }

    ~CECKey() {
        EC_KEY_free(pkey);
    }
    
    EC_KEY* GetECKey() {return pkey;};

    void GetSecretBytes(unsigned char vch[32]) const;

    void SetSecretBytes(const unsigned char vch[32]);

    void GetPrivKey(CPrivKey &privkey, bool fCompressed);

    bool SetPrivKey(const CPrivKey &privkey, bool fSkipCheck=false);

    void GetPubKey(CPubKey &pubkey, bool fCompressed);

    bool SetPubKey(const CPubKey &pubkey);

    bool Sign(const uint256 &hash, std::vector<unsigned char>& vchSig);

    bool Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig);

    bool SignCompact(const uint256 &hash, unsigned char *p64, int &rec);

    // reconstruct public key from a compact signature
    // This is only slightly more CPU intensive than just verifying it.
    // If this function succeeds, the recovered public key is guaranteed to be valid
    // (the signature is a valid signature of the given data for that key)
    bool Recover(const uint256 &hash, const unsigned char *p64, int rec);
    
    bool TweakPublic(const unsigned char vchTweak[32]);
};

bool TweakSecret(unsigned char vchSecretOut[32], const unsigned char vchSecretIn[32], const unsigned char vchTweak[32]);



#endif


