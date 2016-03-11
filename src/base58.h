// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


//
// Why base-58 instead of standard base-64 encoding?
// - Don't want 0OIl characters that look the same in some fonts and
//      could be used to create visually identical looking account numbers.
// - A string with non-alphanumeric characters is not as easily accepted as an account number.
// - E-mail usually won't line-break if there's no punctuation to break at.
// - Double-clicking selects the whole number as one word if it's all alphanumeric.
//
#ifndef BITCOIN_BASE58_H
#define BITCOIN_BASE58_H

#include <string>
#include <vector>
#include <openssl/crypto.h> // for OPENSSL_cleanse()
#include "bignum.h"
#include "key.h"
#include "script.h"

// Encode a byte sequence as a base58-encoded string
std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend);

// Encode a byte vector as a base58-encoded string
std::string EncodeBase58(const std::vector<unsigned char>& vch);

// Decode a base58-encoded string psz into byte vector vchRet
// returns true if decoding is successful
bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet);

// Decode a base58-encoded string str into byte vector vchRet
// returns true if decoding is successful
bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet);

// Encode a byte vector to a base58-encoded string, including checksum
std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn);

// Decode a base58-encoded string psz that includes a checksum, into byte vector vchRet
// returns true if decoding is successful
bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet);

// Decode a base58-encoded string str that includes a checksum, into byte vector vchRet
// returns true if decoding is successful
bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet);

/** Base class for all base58-encoded data */
class CBase58Data
{
protected:
    // the version byte
    unsigned char nVersion;

    // the actually encoded data
    std::vector<unsigned char> vchData;

    CBase58Data();
    ~CBase58Data();

    void SetData(int nVersionIn, const void* pdata, size_t nSize);
    void SetData(int nVersionIn, const unsigned char *pbegin, const unsigned char *pend);

public:
    bool SetString(const char* psz);
    bool SetString(const std::string& str);
    std::string ToString() const;
    const std::vector<unsigned char> &GetData() const;

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
 * Pubkey-pair-addresses have version 1 (or 6 testnet)
 * The data vector contains a serialized copy of two compressed ECDSA secp256k1 public keys.
 */
class CBitcoinAddress;
class CBitcoinAddressVisitor : public boost::static_visitor<bool>
{
private:
    CBitcoinAddress *addr;
public:
    CBitcoinAddressVisitor(CBitcoinAddress *addrIn) : addr(addrIn) { }
    bool operator()(const CKeyID &id) const;
    bool operator()(const CScriptID &id) const;
    bool operator()(const CMalleablePubKey &mpk) const;
    bool operator()(const CNoDestination &no) const;
};

class CBitcoinAddress : public CBase58Data
{
public:
    enum
    {
        PUBKEY_PAIR_ADDRESS = 1,
        PUBKEY_ADDRESS = 8,
        SCRIPT_ADDRESS = 20,
        PUBKEY_PAIR_ADDRESS_TEST = 6,
        PUBKEY_ADDRESS_TEST = 111,
        SCRIPT_ADDRESS_TEST = 196
    };

    bool Set(const CKeyID &id);
    bool Set(const CScriptID &id);
    bool Set(const CTxDestination &dest);
    bool Set(const CMalleablePubKey &mpk);
    bool Set(const CBitcoinAddress &dest);
    bool IsValid() const;

    CBitcoinAddress()
    {
    }

    CBitcoinAddress(const CTxDestination &dest)
    {
        Set(dest);
    }

    CBitcoinAddress(const CMalleablePubKey &mpk)
    {
        Set(mpk);
    }

    CBitcoinAddress(const std::string& strAddress)
    {
        SetString(strAddress);
    }

    CBitcoinAddress(const char* pszAddress)
    {
        SetString(pszAddress);
    }

    CTxDestination Get() const;
    bool GetKeyID(CKeyID &keyID) const;
    bool IsScript() const;
    bool IsPubKey() const;
    bool IsPair() const;
};

bool inline CBitcoinAddressVisitor::operator()(const CKeyID &id) const            { return addr->Set(id); }
bool inline CBitcoinAddressVisitor::operator()(const CScriptID &id) const         { return addr->Set(id); }
bool inline CBitcoinAddressVisitor::operator()(const CMalleablePubKey &mpk) const { return addr->Set(mpk); }
bool inline CBitcoinAddressVisitor::operator()(const CNoDestination &id) const    { return false; }

/** A base58-encoded secret key */
class CBitcoinSecret : public CBase58Data
{
public:
    void SetSecret(const CSecret& vchSecret, bool fCompressed);
    CSecret GetSecret(bool &fCompressedOut);

    bool IsValid() const;

    bool SetString(const char* pszSecret);
    bool SetString(const std::string& strSecret);

    CBitcoinSecret(const CSecret& vchSecret, bool fCompressed);
    CBitcoinSecret()
    {
    }
};

#endif
