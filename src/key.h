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
    unsigned char vch[65];

    unsigned int static GetLen(unsigned char chHeader) {
        if (chHeader == 2 || chHeader == 3)
            return 33;
        if (chHeader == 4 || chHeader == 6 || chHeader == 7)
            return 65;
        return 0;
    }

    unsigned char *begin() {
        return vch;
    }

    friend class CKey;

public:
    CPubKey() { vch[0] = 0xFF; }

    CPubKey(const std::vector<unsigned char> &vchPubKeyIn) {
        int len = vchPubKeyIn.empty() ? 0 : GetLen(vchPubKeyIn[0]);
        if (len) {
            memcpy(vch, &vchPubKeyIn[0], len);
        } else {
            vch[0] = 0xFF;
        }
    }

    unsigned int size() const {
        return GetLen(vch[0]);
    }

    const unsigned char *begin() const {
        return vch;
    }

    const unsigned char *end() const {
        return vch+size();
    }

    friend bool operator==(const CPubKey &a, const CPubKey &b) { return memcmp(a.vch, b.vch, a.size()) == 0; }
    friend bool operator!=(const CPubKey &a, const CPubKey &b) { return memcmp(a.vch, b.vch, a.size()) != 0; }
    friend bool operator<(const CPubKey &a, const CPubKey &b) {
        return a.vch[0] < b.vch[0] ||
               (a.vch[0] == b.vch[0] && memcmp(a.vch+1, b.vch+1, a.size() - 1) < 0);
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        return size() + 1;
    }

    template<typename Stream> void Serialize(Stream &s, int nType, int nVersion) const {
        unsigned int len = size();
        ::Serialize(s, VARINT(len), nType, nVersion);
        s.write((char*)vch, len);
    }

    template<typename Stream> void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int len;
        ::Unserialize(s, VARINT(len), nType, nVersion);
        if (len <= 65) {
            s.read((char*)vch, len);
        } else {
            // invalid pubkey
            vch[0] = 0xFF;
            char dummy;
            while (len--)
                s.read(&dummy, 1);
        }
    }

    CKeyID GetID() const {
        return CKeyID(Hash160(vch, vch+size()));
    }

    uint256 GetHash() const {
        return Hash(vch, vch+size());
    }

    bool IsValid() const {
        return size() > 0;
    }

    bool IsCompressed() const {
        return size() == 33;
    }

    std::vector<unsigned char> Raw() const {
        return std::vector<unsigned char>(vch, vch+size());
    }
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

public:
    void SetCompressedPubKey(bool fCompressed = true);

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
};

#endif
