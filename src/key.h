// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEY_H
#define BITCOIN_KEY_H

#include <stdexcept>
#include <vector>

#include "allocators.h"
#include "serialize.h"
#include "uint256.h"
#include "hash.h"
#include "bignum.h"
#include "ies.h"

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

/** A reference to a CKey: the Hash160 of its serialized public key */
class CKeyID : public uint160
{
public:
    CKeyID() : uint160(0) { }
    CKeyID(const uint160 &in) : uint160(in) { }
};

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID : public uint160
{
public:
    CScriptID() : uint160(0) { }
    CScriptID(const uint160 &in) : uint160(in) { }
};

/** An encapsulated public key. */
class CPubKey {
private:
    std::vector<unsigned char> vchPubKey;
    friend class CKey;

public:
    CPubKey() { }
    CPubKey(const std::vector<unsigned char> &vchPubKeyIn) : vchPubKey(vchPubKeyIn) { }
    friend bool operator==(const CPubKey &a, const CPubKey &b) { return a.vchPubKey == b.vchPubKey; }
    friend bool operator!=(const CPubKey &a, const CPubKey &b) { return a.vchPubKey != b.vchPubKey; }
    friend bool operator<(const CPubKey &a, const CPubKey &b) { return a.vchPubKey < b.vchPubKey; }

    IMPLEMENT_SERIALIZE(
        READWRITE(vchPubKey);
    )

    CKeyID GetID() const {
        return CKeyID(Hash160(vchPubKey));
    }

    uint256 GetHash() const {
        return Hash(vchPubKey.begin(), vchPubKey.end());
    }

    bool IsValid() const {
        return vchPubKey.size() == 33 || vchPubKey.size() == 65;
    }

    bool IsCompressed() const {
        return vchPubKey.size() == 33;
    }

    std::vector<unsigned char> Raw() const {
        return vchPubKey;
    }

    // Encrypt data
    void EncryptData(const std::vector<unsigned char>& data, std::vector<unsigned char>& encrypted);
};


// secure_allocator is defined in allocators.h
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
    bool SetPubKey(const CPubKey& vchPubKey);
    CPubKey GetPubKey() const;

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

    // Check whether an element of a signature (r or s) is valid.
    static bool CheckSignatureElement(const unsigned char *vch, int len, bool half);

    // Reserialize to DER
    static bool ReserealizeSignature(std::vector<unsigned char>& vchSig);

    // Encrypt data
    void EncryptData(const std::vector<unsigned char>& data, std::vector<unsigned char>& encrypted);

    // Decrypt data
    void DecryptData(const std::vector<unsigned char>& encrypted, std::vector<unsigned char>& data);
};

class CPoint
{
private:
    EC_POINT *point;
    EC_GROUP* group;
    BN_CTX* ctx;

public:
    CPoint();
    bool operator!=(const CPoint &a);
    ~CPoint();

    // Initialize from octets stream
    bool setBytes(const std::vector<unsigned char> &vchBytes);

    // Serialize to octets stream
    bool getBytes(std::vector<unsigned char> &vchBytes);

    // ECC multiplication by specified multiplier
    bool ECMUL(const CBigNum &bnMultiplier);

    // Calculate G*m + q
    bool ECMULGEN(const CBigNum &bnMultiplier, const CPoint &qPoint);

    bool IsInfinity() { return EC_POINT_is_at_infinity(group, point); }
};

class CMalleablePubKey
{
private:
    unsigned char nVersion;
    CPubKey pubKeyL;
    CPubKey pubKeyH;
    friend class CMalleableKey;

    static const unsigned char CURRENT_VERSION = 1;

public:
    CMalleablePubKey() { nVersion = CMalleablePubKey::CURRENT_VERSION; }
    CMalleablePubKey(const std::string& strMalleablePubKey) { SetString(strMalleablePubKey); }
    CMalleablePubKey(const CPubKey &pubKeyInL, const CPubKey &pubKeyInH) : pubKeyL(pubKeyInL), pubKeyH(pubKeyInH) { nVersion = CMalleablePubKey::CURRENT_VERSION; }
    CMalleablePubKey(const std::vector<unsigned char> &pubKeyInL, const std::vector<unsigned char> &pubKeyInH) : pubKeyL(pubKeyInL), pubKeyH(pubKeyInH) { nVersion = CMalleablePubKey::CURRENT_VERSION; }

    IMPLEMENT_SERIALIZE(
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(pubKeyL);
        READWRITE(pubKeyH);
    )

    bool IsValid() const {
        return pubKeyL.IsValid() && pubKeyH.IsValid();
    }

    bool operator==(const CMalleablePubKey &b);
    bool operator!=(const CMalleablePubKey &b) { return !(*this == b); }

    std::string ToString();
    bool SetString(const std::string& strMalleablePubKey);
    uint256 GetID() const;

    CPubKey& GetL() { return pubKeyL; }
    CPubKey& GetH() { return pubKeyH; }
    void GetVariant(CPubKey &R, CPubKey &vchPubKeyVariant);
};

class CMalleableKey
{
private:
    unsigned char nVersion;
    CSecret vchSecretL;
    CSecret vchSecretH;

    friend class CMalleableKeyView;

    static const unsigned char CURRENT_VERSION = 1;

public:
    CMalleableKey();
    CMalleableKey(const CMalleableKey &b);
    CMalleableKey(const CSecret &L, const CSecret &H);
    CMalleableKey& operator=(const CMalleableKey &b);
    ~CMalleableKey();

    IMPLEMENT_SERIALIZE(
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(vchSecretL);
        READWRITE(vchSecretH);
    )

    std::string ToString();
    bool SetString(const std::string& strMalleablePubKey);

    void Reset();
    void MakeNewKeys();
    bool IsNull() const;
    bool SetSecrets(const CSecret &pvchSecretL, const CSecret &pvchSecretH);
    void GetSecrets(CSecret &pvchSecretL, CSecret &pvchSecretH) const;

    CMalleablePubKey GetMalleablePubKey() const;
    bool CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant);
    bool CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant, CKey &privKeyVariant);
};

class CMalleableKeyView
{
private:
    unsigned char nVersion;
    CSecret vchSecretL;
    std::vector<unsigned char> vchPubKeyH;

    static const unsigned char CURRENT_VERSION = 1;

public:
    CMalleableKeyView() { nVersion = 0; };
    CMalleableKeyView(const CMalleableKey &b);
    CMalleableKeyView(const CSecret &L, const CPubKey &pvchPubKeyH);

    CMalleableKeyView(const CMalleableKeyView &b);
    CMalleableKeyView& operator=(const CMalleableKey &b);
    ~CMalleableKeyView();


    IMPLEMENT_SERIALIZE(
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(vchSecretL);
        READWRITE(vchPubKeyH);
    )

    bool IsNull() const;
    std::string ToString();
    bool SetString(const std::string& strMalleablePubKey);

    CMalleablePubKey GetMalleablePubKey() const;
    bool CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant);
};

#endif
