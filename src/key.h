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

/** An encapsulated OpenSSL Elliptic Curve key (public) */
class CPubKey
{
private:

    /**
     * Just store the serialized data.
     * Its length can very cheaply be computed from the first byte.
     */
    unsigned char vbytes[65];

    //! Compute the length of a pubkey with a given first byte.
    unsigned int static GetLen(unsigned char chHeader)
    {
        if (chHeader == 2 || chHeader == 3)
            return 33;
        if (chHeader == 4 || chHeader == 6 || chHeader == 7)
            return 65;
        return 0;
    }

    // Set this key data to be invalid
    void Invalidate()
    {
        vbytes[0] = 0xFF;
    }

public:
    // Construct an invalid public key.
    CPubKey()
    {
        Invalidate();
    }

    // Initialize a public key using begin/end iterators to byte data.
    template <typename T>
    void Set(const T pbegin, const T pend)
    {
        int len = pend == pbegin ? 0 : GetLen(pbegin[0]);
        if (len && len == (pend - pbegin))
            memcpy(vbytes, (unsigned char*)&pbegin[0], len);
        else
            Invalidate();
    }

    void Set(const std::vector<unsigned char>& vch)
    {
        Set(vch.begin(), vch.end());
    }

    template <typename T>
    CPubKey(const T pbegin, const T pend)
    {
        Set(pbegin, pend);
    }

    CPubKey(const std::vector<unsigned char>& vch)
    {
        Set(vch.begin(), vch.end());
    }

    // Read-only vector-like interface to the data.
    unsigned int size() const { return GetLen(vbytes[0]); }
    const unsigned char* begin() const { return vbytes; }
    const unsigned char* end() const { return vbytes + size(); }
    const unsigned char& operator[](unsigned int pos) const { return vbytes[pos]; }

    friend bool operator==(const CPubKey& a, const CPubKey& b) { return a.vbytes[0] == b.vbytes[0] && memcmp(a.vbytes, b.vbytes, a.size()) == 0; }
    friend bool operator!=(const CPubKey& a, const CPubKey& b) { return !(a == b); }
    friend bool operator<(const CPubKey& a, const CPubKey& b)  { return a.vbytes[0] < b.vbytes[0] || (a.vbytes[0] == b.vbytes[0] && memcmp(a.vbytes, b.vbytes, a.size()) < 0); }

    //! Implement serialization, as if this was a byte vector.
    unsigned int GetSerializeSize(int nType, int nVersion) const
    {
        return size() + 1;
    }
    template <typename Stream>
    void Serialize(Stream& s, int nType, int nVersion) const
    {
        unsigned int len = size();
        ::WriteCompactSize(s, len);
        s.write((char*)vbytes, len);
    }
    template <typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        unsigned int len = ::ReadCompactSize(s);
        if (len <= 65) {
            s.read((char*)vbytes, len);
        } else {
            // invalid pubkey, skip available data
            char dummy;
            while (len--)
                s.read(&dummy, 1);
            Invalidate();
        }
    }

    CKeyID GetID() const
    {
        return CKeyID(Hash160(vbytes, vbytes + size()));
    }

    uint256 GetHash() const
    {
        return Hash(vbytes, vbytes + size());
    }

    /*
     * Check syntactic correctness.
     *
     * Note that this is consensus critical as CheckSig() calls it!
     */
    bool IsValid() const
    {
        return size() > 0;
    }

    //! fully validate whether this is a valid public key (more expensive than IsValid())
    bool IsFullyValid() const
    {
        const unsigned char* pbegin = &vbytes[0];
        EC_KEY *pkey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (o2i_ECPublicKey(&pkey, &pbegin, size()))
        {
            EC_KEY_free(pkey);
            return true;
        }
        return false;
    }

    //! Check whether this is a compressed public key.
    bool IsCompressed() const
    {
        return size() == 33;
    }

    bool Verify(const uint256& hash, const std::vector<unsigned char>& vchSig) const;
    bool VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig);

    bool SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig);

    // Reserialize to DER
    static bool ReserealizeSignature(std::vector<unsigned char>& vchSig);

    // Encrypt data
    void EncryptData(const std::vector<unsigned char>& data, std::vector<unsigned char>& encrypted);
};

// secure_allocator is defined in allocators.h
// CPrivKey is a serialized private key, with all parameters included (279 bytes)
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CPrivKey;
// CSecret is a serialization of just the secret parameter (32 bytes)
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CSecret;

/** An encapsulated OpenSSL Elliptic Curve key (private) */
class CKey
{
protected:
    EC_KEY* pkey;
    bool fSet;

public:

    void Reset();

    CKey();
    CKey(const CKey& b);
    CKey(const CSecret& b, bool fCompressed=true);

    CKey& operator=(const CKey& b);

    ~CKey();

    bool IsNull() const;
    bool IsCompressed() const;

    void SetCompressedPubKey(bool fCompressed=true);
    void MakeNewKey(bool fCompressed=true);
    bool SetPrivKey(const CPrivKey& vchPrivKey);
    bool SetSecret(const CSecret& vchSecret, bool fCompressed = true);
    CSecret GetSecret(bool &fCompressed) const;
    CSecret GetSecret() const;
    CPrivKey GetPrivKey() const;
    CPubKey GetPubKey() const;
    bool WritePEM(BIO *streamObj, const SecureString &strPassKey) const;

    bool Sign(uint256 hash, std::vector<unsigned char>& vchSig);

    // create a compact signature (65 bytes), which allows reconstructing the used public key
    // The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
    // The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
    //                  0x1D = second key with even y, 0x1E = second key with odd y
    bool SignCompact(uint256 hash, std::vector<unsigned char>& vchSig);

    bool IsValid();

    // Check whether an element of a signature (r or s) is valid.
    static bool CheckSignatureElement(const unsigned char *vch, int len, bool half);

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
