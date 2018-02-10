// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Why base-58 instead of standard base-64 encoding?
 * - Don't want 0OIl characters that look the same in some fonts and
 *      could be used to create visually identical looking data.
 * - A string with non-alphanumeric characters is not as easily accepted as input.
 * - E-mail usually won't line-break if there's no punctuation to break at.
 * - Double-clicking selects the whole string as one word if it's all alphanumeric.
 */
#ifndef BITCOIN_BASE58_H
#define BITCOIN_BASE58_H

#include <chainparams.h>
#include <key.h>
#include <key/extkey.h>
#include <key/stealth.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <support/allocators/zeroafterfree.h>
#include <bech32.h>

#include <string>
#include <vector>

/**
 * Encode a byte sequence as a base58-encoded string.
 * pbegin and pend cannot be nullptr, unless both are.
 */
std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend);

/**
 * Encode a byte vector as a base58-encoded string
 */
std::string EncodeBase58(const std::vector<unsigned char>& vch);

/**
 * Decode a base58-encoded string (psz) into a byte vector (vchRet).
 * return true if decoding is successful.
 * psz cannot be nullptr.
 */
bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet);

/**
 * Decode a base58-encoded string (str) into a byte vector (vchRet).
 * return true if decoding is successful.
 */
bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet);

/**
 * Encode a byte vector into a base58-encoded string, including checksum
 */
std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn);

/**
 * Decode a base58-encoded string (psz) that includes a checksum into a byte
 * vector (vchRet), return true if decoding is successful
 */
inline bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet);

/**
 * Decode a base58-encoded string (str) that includes a checksum into a byte
 * vector (vchRet), return true if decoding is successful
 */
inline bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet);

/**
 * Base class for all base58-encoded data
 */
class CBase58Data
{
protected:
    //! the version byte(s)
    std::vector<unsigned char> vchVersion;

    //! the actually encoded data
    typedef std::vector<unsigned char, zero_after_free_allocator<unsigned char> > vector_uchar;
    vector_uchar vchData;
    bool fBech32;

    CBase58Data();
    void SetData(const std::vector<unsigned char> &vchVersionIn, const void* pdata, size_t nSize);
    void SetData(const std::vector<unsigned char> &vchVersionIn, const unsigned char *pbegin, const unsigned char *pend);

public:
    bool SetString(const char* psz, unsigned int nVersionBytes = 1);
    bool SetString(const std::string& str);
    std::string ToString() const;
    int CompareTo(const CBase58Data& b58) const;

    bool operator==(const CBase58Data& b58) const { return CompareTo(b58) == 0; }
    bool operator<=(const CBase58Data& b58) const { return CompareTo(b58) <= 0; }
    bool operator>=(const CBase58Data& b58) const { return CompareTo(b58) >= 0; }
    bool operator< (const CBase58Data& b58) const { return CompareTo(b58) <  0; }
    bool operator> (const CBase58Data& b58) const { return CompareTo(b58) >  0; }
};

/** base58-encoded Bitcoin addresses.
 * Public-key-hash-addresses have version 0 (or 111 testnet).
 * The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
 * Script-hash-addresses have version 5 (or 196 testnet).
 * The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
 */
class CBitcoinAddress : public CBase58Data {
public:
    bool Set(const CKeyID &id, bool fBech32 = false);
    bool Set(const CScriptID &id, bool fBech32 = false);
    bool Set(const CKeyID256 &id, bool fBech32 = false);
    bool Set(const CScriptID256 &id, bool fBech32 = false);
    bool Set(const CKeyID &id, CChainParams::Base58Type prefix, bool fBech32 = false);
    bool Set(const CStealthAddress &sx, bool fBech32 = false);
    bool Set(const CExtKeyPair &ek, bool fBech32 = false);
    bool Set(const CTxDestination &dest, bool fBech32 = false);

    bool IsValidStealthAddress() const;
    bool IsValidStealthAddress(const CChainParams &params) const;
    bool IsValid() const;
    bool IsValid(const CChainParams &params) const;
    bool IsValid(CChainParams::Base58Type prefix) const;

    CBitcoinAddress() {}
    CBitcoinAddress(const CTxDestination &dest) { Set(dest); }
    CBitcoinAddress(const CTxDestination &dest, bool fBech32) { Set(dest, fBech32); }
    CBitcoinAddress(const std::string& strAddress) { SetString(strAddress); }
    CBitcoinAddress(const char* pszAddress) { SetString(pszAddress); }

    CTxDestination Get() const;
    bool GetKeyID(CKeyID &keyID) const;
    bool GetKeyID(CKeyID256 &keyID) const;
    bool GetKeyID(CKeyID &keyID, CChainParams::Base58Type prefix) const;
    bool GetIndexKey(uint256 &hashBytes, int &type) const;
    bool IsScript() const;
};

/**
 * A base58-encoded secret key
 */
class CBitcoinSecret : public CBase58Data
{
public:
    void SetKey(const CKey& vchSecret);
    CKey GetKey() const;
    bool IsValid() const;
    bool SetString(const char* pszSecret);
    bool SetString(const std::string& strSecret);

    CBitcoinSecret(const CKey& vchSecret) { SetKey(vchSecret); }
    CBitcoinSecret() {}
};

template<typename K, int Size, CChainParams::Base58Type Type> class CBitcoinExtKeyBase : public CBase58Data
{
public:
    void SetKey(const K &key) {
        unsigned char vch[Size];
        key.Encode(vch);
        SetData(Params().Base58Prefix(Type), vch, vch+Size);
    }

    int Set58(const char *base58)
    {
        std::vector<uint8_t> vchBytes;
        if (!DecodeBase58(base58, vchBytes))
            return 1;

        if (vchBytes.size() != BIP32_KEY_LEN)
            return 2;

        if (!VerifyChecksum(vchBytes))
            return 3;

        if (0 != memcmp(&vchBytes[0], &Params().Base58Prefix(Type)[0], 4))
            return 4;

        SetData(Params().Base58Prefix(Type), &vchBytes[4], &vchBytes[4]+Size);
        return 0;
    }

    K GetKey() {
        K ret;
        if (vchData.size() == Size) {
            // If base58 encoded data does not hold an ext key, return a !IsValid() key
            ret.Decode(vchData.data());
        }
        return ret;
    }

    CBitcoinExtKeyBase(const K &key) {
        SetKey(key);
    }

    CBitcoinExtKeyBase(const std::string& strBase58c) {
        SetString(strBase58c.c_str(), Params().Base58Prefix(Type).size());
    }

    CBitcoinExtKeyBase() {}
};

typedef CBitcoinExtKeyBase<CExtKey, BIP32_EXTKEY_SIZE, CChainParams::EXT_SECRET_KEY> CBitcoinExtKey;
typedef CBitcoinExtKeyBase<CExtPubKey, BIP32_EXTKEY_SIZE, CChainParams::EXT_PUBLIC_KEY> CBitcoinExtPubKey;


class CExtKey58 : public CBase58Data
{
public:
    CExtKey58() {};

    CExtKey58(const CExtKeyPair &key, CChainParams::Base58Type type)
    {
        SetKey(key, type);
    };

    void SetKeyV(const CExtKeyPair &key)
    {
        SetKey(key, CChainParams::EXT_SECRET_KEY);
    };

    void SetKeyP(const CExtKeyPair &key)
    {
        SetKey(key, CChainParams::EXT_PUBLIC_KEY);
    };

    void SetKey(const CExtKeyPair &key, CChainParams::Base58Type type)
    {
        uint8_t vch[74];

        switch (type)
        {
            case CChainParams::EXT_SECRET_KEY:
            case CChainParams::EXT_SECRET_KEY_BTC:
                key.EncodeV(vch);
                break;
            //case CChainParams::EXT_PUBLIC_KEY:
            //case CChainParams::EXT_PUBLIC_KEY_BTC:
            default:
                key.EncodeP(vch);
                break;
        };

        SetData(Params().Base58Prefix(type), vch, vch+74);
    };

    CExtKeyPair GetKey()
    {
        CExtKeyPair rv;
        if (vchVersion == Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)
            || vchVersion == Params().Base58Prefix(CChainParams::EXT_SECRET_KEY_BTC))
        {
            rv.DecodeV(&vchData[0]);
            return rv;
        };
        rv.DecodeP(&vchData[0]);
        return rv;
    };

    bool GetPubKey(CExtPubKey &rv, const CChainParams *pparams)
    {
        if (vchVersion == pparams->Base58Prefix(CChainParams::EXT_SECRET_KEY)
            || vchVersion == pparams->Base58Prefix(CChainParams::EXT_SECRET_KEY_BTC))
        {
            CExtKey ek;
            ek.Decode(&vchData[0]);
            rv = ek.Neutered();
            return true;
        };

        if (vchVersion == pparams->Base58Prefix(CChainParams::EXT_PUBLIC_KEY)
            || vchVersion == pparams->Base58Prefix(CChainParams::EXT_PUBLIC_KEY_BTC))
        {
            rv.Decode(&vchData[0]);
            return true;
        }

        return false;
    };

    int Set58(const char *base58);
    int Set58(const char *base58, CChainParams::Base58Type type, const CChainParams *pparams);

    bool IsValid(CChainParams::Base58Type prefix) const;

    std::string ToStringVersion(CChainParams::Base58Type prefix);
};

std::string EncodeDestination(const CTxDestination& dest, bool fBech32=false);
CTxDestination DecodeDestination(const std::string& str);
bool IsValidDestinationString(const std::string& str);
bool IsValidDestinationString(const std::string& str, const CChainParams& params);


#endif // BITCOIN_BASE58_H
