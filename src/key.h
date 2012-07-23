// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEY_H
#define BITCOIN_KEY_H

#include <stdexcept>
#include <vector>

#include "allocators.h"
#include "uint256.h"

#include <openssl/ec.h> // for EC_KEY definition

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

/** An encapsulated OpenSSL Elliptic Curve key (public and/or private) */
class CKey
{
protected:
    EC_KEY* pkey;
    bool fSet;
    bool fCompressedPubKey;

    void SetCompressedPubKey();

public:

    void Reset();

    CKey();
    CKey(const CKey& b);

    CKey& operator=(const CKey& b);

    ~CKey();

    bool IsNull() const;
    bool IsCompressed() const;

    void MakeNewKey(bool fCompressed);
    bool SetPrivKey(const CPrivKey& vchPrivKey);
    bool SetSecret(const CSecret& vchSecret, bool fCompressed = false);
    CSecret GetSecret(bool &fCompressed) const;
    CPrivKey GetPrivKey() const;
    bool SetPubKey(const std::vector<unsigned char>& vchPubKey);
    std::vector<unsigned char> GetPubKey() const;

    bool Sign(uint256 hash, std::vector<unsigned char>& vchSig);

    // create a compact signature (65 bytes), which allows reconstructing the used public key
    // The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
    // The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
    //                  0x1D = second key with even y, 0x1E = second key with odd y
    bool SignCompact(uint256 hash, std::vector<unsigned char>& vchSig);

    // reconstruct public key from a compact signature
    // This is only slightly more CPU intensive than just verifying it.
    // If this function succeeds, the recovered public key is guaranteed to be valid
    // (the signature is a valid signature of the given data for that key)
    bool SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig);

    bool Verify(uint256 hash, const std::vector<unsigned char>& vchSig);

    // Verify a compact signature
    bool VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig);

    bool IsValid();
};

#endif
