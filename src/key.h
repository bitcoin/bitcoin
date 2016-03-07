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
    CKey(const CSecret& b, bool fCompressed=true);

    CKey& operator=(const CKey& b);

    ~CKey();

    bool IsNull() const;
    bool IsCompressed() const;

    void MakeNewKey(bool fCompressed=true);
    bool SetPrivKey(const CPrivKey& vchPrivKey);
    bool SetSecret(const CSecret& vchSecret, bool fCompressed = true);
    CSecret GetSecret(bool &fCompressed) const;
    CSecret GetSecret() const;
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

    // Initialize from pubkey
    bool setPubKey(const CPubKey &vchPubKey);

    // Serialize to octets stream
    bool getBytes(std::vector<unsigned char> &vchBytes);

    // ECC multiplication by specified multiplier
    bool ECMUL(const CBigNum &bnMultiplier);

    // Calculate G*m + q
    bool ECMULGEN(const CBigNum &bnMultiplier, const CPoint &qPoint);

    bool IsInfinity() { return EC_POINT_is_at_infinity(group, point) != 0; }
};

class CMalleablePubKey
{
private:
    CPubKey pubKeyL;
    CPubKey pubKeyH;
    friend class CMalleableKey;

    static const unsigned char CURRENT_VERSION = 1;

public:
    CMalleablePubKey() { }
    CMalleablePubKey(const CMalleablePubKey& mpk)
    {
        pubKeyL = mpk.pubKeyL;
        pubKeyH = mpk.pubKeyH;
    }
    CMalleablePubKey(const std::vector<unsigned char> &vchPubKeyPair) { setvch(vchPubKeyPair); }
    CMalleablePubKey(const std::string& strMalleablePubKey) { SetString(strMalleablePubKey); }
    CMalleablePubKey(const CPubKey &pubKeyInL, const CPubKey &pubKeyInH) : pubKeyL(pubKeyInL), pubKeyH(pubKeyInH) { }

    IMPLEMENT_SERIALIZE(
        READWRITE(pubKeyL);
        READWRITE(pubKeyH);
    )

    bool IsValid() const {
        return pubKeyL.IsValid() && pubKeyH.IsValid();
    }

    bool operator==(const CMalleablePubKey &b);
    bool operator!=(const CMalleablePubKey &b) { return !(*this == b); }
    CMalleablePubKey& operator=(const CMalleablePubKey& mpk) {
        pubKeyL = mpk.pubKeyL;
        pubKeyH = mpk.pubKeyH;
        return *this;
    }

    std::string ToString() const;
    bool SetString(const std::string& strMalleablePubKey);

    CKeyID GetID() const {
        return pubKeyL.GetID();
    }

    bool setvch(const std::vector<unsigned char> &vchPubKeyPair);
    std::vector<unsigned char> Raw() const;

    CPubKey& GetL() { return pubKeyL; }
    CPubKey& GetH() { return pubKeyH; }
    void GetVariant(CPubKey &R, CPubKey &vchPubKeyVariant);
};

class CMalleableKey
{
private:
    CSecret vchSecretL;
    CSecret vchSecretH;

    friend class CMalleableKeyView;

public:
    CMalleableKey();
    CMalleableKey(const CMalleableKey &b);
    CMalleableKey(const CSecret &L, const CSecret &H);
    ~CMalleableKey();

    IMPLEMENT_SERIALIZE(
        READWRITE(vchSecretL);
        READWRITE(vchSecretH);
    )

    std::string ToString() const;
    bool SetString(const std::string& strMalleablePubKey);
    std::vector<unsigned char> Raw() const;
    CMalleableKey& operator=(const CMalleableKey& mk) {
        vchSecretL = mk.vchSecretL;
        vchSecretH = mk.vchSecretH;
        return *this;
    }

    void Reset();
    void MakeNewKeys();
    bool IsNull() const;
    bool IsValid() const { return !IsNull() && GetMalleablePubKey().IsValid(); }
    bool SetSecrets(const CSecret &pvchSecretL, const CSecret &pvchSecretH);

    CSecret GetSecretL() const { return vchSecretL; }
    CSecret GetSecretH() const { return vchSecretH; }

    CKeyID GetID() const {
        return GetMalleablePubKey().GetID();
    }
    CMalleablePubKey GetMalleablePubKey() const;
    bool CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant) const;
    bool CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant, CKey &privKeyVariant) const;
};

class CMalleableKeyView
{
private:
    CSecret vchSecretL;
    CPubKey vchPubKeyH;

public:
    CMalleableKeyView() { };
    CMalleableKeyView(const CMalleableKey &b);
    CMalleableKeyView(const std::string &strMalleableKey);

    CMalleableKeyView(const CMalleableKeyView &b);
    CMalleableKeyView& operator=(const CMalleableKey &b);
    ~CMalleableKeyView();

    IMPLEMENT_SERIALIZE(
        READWRITE(vchSecretL);
        READWRITE(vchPubKeyH);
    )

    bool IsValid() const;
    std::string ToString() const;
    bool SetString(const std::string& strMalleablePubKey);
    std::vector<unsigned char> Raw() const;
    CMalleableKeyView& operator=(const CMalleableKeyView& mkv) {
        vchSecretL = mkv.vchSecretL;
        vchPubKeyH = mkv.vchPubKeyH;
        return *this;
    }

    CKeyID GetID() const {
        return GetMalleablePubKey().GetID();
    }
    CMalleablePubKey GetMalleablePubKey() const;
    CMalleableKey GetMalleableKey(const CSecret &vchSecretH) const { return CMalleableKey(vchSecretL, vchSecretH); }
    bool CheckKeyVariant(const CPubKey &R, const CPubKey &vchPubKeyVariant) const;

    bool operator <(const CMalleableKeyView& kv) const { return vchPubKeyH.GetID() < kv.vchPubKeyH.GetID(); }
};

#endif
