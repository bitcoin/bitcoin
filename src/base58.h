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
// This base-58 codec encodes a sequence of bytes, not just a big-endian number.
// The difference is that we preserve the exact number of leading 0 bytes.
// Each leading zero is represented by a leading 0-value base-58 digit ('1').
// The remaining bytes, starting from the first non-zero byte, are then interpreted
// as a big-endian binary number and converted into a big-endian base-58 number.
//
// Example:
// base-58 encoded:  "127"  ==  binary: 0x00 0x40
// "1" leading 0-byte --------------------^^   ^^
// "2" 1-valued base-58 digit ----> 1*58 + 6 = 64
// "7" 6-valued base-58 digit -------------^
//
#ifndef BITCOIN_BASE58_H
#define BITCOIN_BASE58_H

#include <string>
#include <vector>

#include "bignum.h"
#include "key.h"
#include "script.h"
#include "allocators.h"

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// Encode a byte sequence as a base58-encoded string
inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
    const unsigned char* p;

    // Convert bignum to std::string
    std::string str;
    // Expected size increase from base58 conversion is log(256)/log(58) approximately 1.36566
    // use 350/256 to be safe
    str.reserve(((pend - pbegin) * 350) / 256 + 1);

    // Leading zeros encoded as base58 zeros
    for (p = pbegin; p < pend && *p == 0; p++)
        str += pszBase58[0];
    ptrdiff_t nLeadingZeros = p - pbegin;

    // Convert big endian data to bignum
    CBigNum bn;
    BN_bin2bn(p, pend - p, &bn);

    while (!!bn)
    {
        BN_ULONG rem = BN_div_word(&bn, 58);
        if (rem == (BN_ULONG) -1)
            throw bignum_error("EncodeBase58 : BN_div_word failed");
        str += pszBase58[rem];
    }

    // Convert little endian std::string after leading zeros to big endian
    reverse(str.begin() + nLeadingZeros, str.end());
    return str;
}

// Encode a byte vector as a base58-encoded string
inline std::string EncodeBase58(const std::vector<unsigned char>& vch)
{
    return EncodeBase58(&vch[0], &vch[0] + vch.size());
}

// Decode a base58-encoded string psz into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet)
{
    // use unsigned char as array index
    const unsigned char* p = (const unsigned char*)psz;
    CBigNum bn = 0;

    // map base58 digit to number, BAD, or SPACE
    enum RBASE58 {
        // 0 .. 57			// base58 digit of value 0 .. 57
        RBASE58_BAD   = -1,		// neither base58, nor white space
        RBASE58_SPACE = -2		// space, tab, newline, vtab, form feed, carriage return
    };
    static const signed char rgi8RBase58[256] =
        {-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-1,-1,-1,-1,-1,-1,
         -1, 9,10,11,12,13,14,15,16,-1,17,18,19,20,21,-1,
         22,23,24,25,26,27,28,29,30,31,32,-1,-1,-1,-1,-1,
         -1,33,34,35,36,37,38,39,40,41,42,43,-1,44,45,46,
         47,48,49,50,51,52,53,54,55,56,57,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };

    // The above initializer was calculated using:
    //    memset(rgi8RBase58, RBASE58_BAD, 256);
    //    rgi8RBase58[' ']  = RBASE58_SPACE;
    //    rgi8RBase58['\t'] = RBASE58_SPACE;
    //    rgi8RBase58['\n'] = RBASE58_SPACE;
    //    rgi8RBase58['\v'] = RBASE58_SPACE;
    //    rgi8RBase58['\f'] = RBASE58_SPACE;
    //    rgi8RBase58['\r'] = RBASE58_SPACE;
    //
    //    for (int i = 0; i < 58; i++)
    //        rgi8RBase58[(unsigned char)pszBase58[i]] = i;

    // Skip whitespace
    while (rgi8RBase58[*p] == RBASE58_SPACE)
        p++;

    // Count leading zeros
    int nLeadingZeros;
    for (nLeadingZeros = 0; *p == pszBase58[0]; p++)
        nLeadingZeros++;

    // Convert big endian string to bignum
    int v;
    while ((v = rgi8RBase58[*p]) >= 0)
    {
        bn *= 58;
        bn += v;
        p++;
    }

    // Skip whitespace after base58 string
    while (rgi8RBase58[*p] == RBASE58_SPACE)
        p++;

    // Fail if there is junk at the end
    if (*p != '\0')
        return false;

    // Fill in leading zeros and make space for bn
    vchRet.assign(nLeadingZeros + BN_num_bytes(&bn), 0);

    // Fill big endian bn into the right place
    BN_bn2bin(&bn, &vchRet[nLeadingZeros]);

    return true;
}

// Decode a base58-encoded string str into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet)
{
    return DecodeBase58(str.c_str(), vchRet);
}




// Encode a byte vector to a base58-encoded string, including checksum
inline std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn)
{
    // add 4-byte hash check to the end
    std::vector<unsigned char> vch(vchIn);
    uint256 hash = Hash(vch.begin(), vch.end());
    vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
    return EncodeBase58(vch);
}

// Decode a base58-encoded string psz that includes a checksum, into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet)
{
    if (!DecodeBase58(psz, vchRet))
        return false;
    if (vchRet.size() < 4)
    {
        vchRet.clear();
        return false;
    }
    uint256 hash = Hash(vchRet.begin(), vchRet.end()-4);
    if (memcmp(&hash, &vchRet.end()[-4], 4) != 0)
    {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size()-4);
    return true;
}

// Decode a base58-encoded string str that includes a checksum, into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet)
{
    return DecodeBase58Check(str.c_str(), vchRet);
}





/** Base class for all base58-encoded data */
class CBase58Data
{
protected:
    // the version byte
    unsigned char nVersion;

    // the actually encoded data
    typedef std::vector<unsigned char, zero_after_free_allocator<unsigned char> > vector_uchar;
    vector_uchar vchData;

    CBase58Data()
    {
        nVersion = 0;
        vchData.clear();
    }

    void SetData(int nVersionIn, const void* pdata, size_t nSize)
    {
        nVersion = nVersionIn;
        vchData.resize(nSize);
        if (!vchData.empty())
            memcpy(&vchData[0], pdata, nSize);
    }

    void SetData(int nVersionIn, const unsigned char *pbegin, const unsigned char *pend)
    {
        SetData(nVersionIn, (void*)pbegin, pend - pbegin);
    }

public:
    bool SetString(const char* psz)
    {
        std::vector<unsigned char> vchTemp;
        DecodeBase58Check(psz, vchTemp);
        if (vchTemp.empty())
        {
            vchData.clear();
            nVersion = 0;
            return false;
        }
        nVersion = vchTemp[0];
        vchData.resize(vchTemp.size() - 1);
        if (!vchData.empty())
            memcpy(&vchData[0], &vchTemp[1], vchData.size());
        OPENSSL_cleanse(&vchTemp[0], vchData.size());
        return true;
    }

    bool SetString(const std::string& str)
    {
        return SetString(str.c_str());
    }

    std::string ToString() const
    {
        std::vector<unsigned char> vch(1, nVersion);
        vch.insert(vch.end(), vchData.begin(), vchData.end());
        return EncodeBase58Check(vch);
    }

    int CompareTo(const CBase58Data& b58) const
    {
        if (nVersion < b58.nVersion) return -1;
        if (nVersion > b58.nVersion) return  1;
        if (vchData < b58.vchData)   return -1;
        if (vchData > b58.vchData)   return  1;
        return 0;
    }

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
class CBitcoinAddress;
class CBitcoinAddressVisitor : public boost::static_visitor<bool>
{
private:
    CBitcoinAddress *addr;
public:
    CBitcoinAddressVisitor(CBitcoinAddress *addrIn) : addr(addrIn) { }
    bool operator()(const CKeyID &id) const;
    bool operator()(const CScriptID &id) const;
    bool operator()(const CNoDestination &no) const;
};

class CBitcoinAddress : public CBase58Data
{
public:
    enum
    {
        PUBKEY_ADDRESS = 0,
        SCRIPT_ADDRESS = 5,
        PUBKEY_ADDRESS_TEST = 111,
        SCRIPT_ADDRESS_TEST = 196,
    };

    bool Set(const CKeyID &id) {
        SetData(fTestNet ? PUBKEY_ADDRESS_TEST : PUBKEY_ADDRESS, &id, 20);
        return true;
    }

    bool Set(const CScriptID &id) {
        SetData(fTestNet ? SCRIPT_ADDRESS_TEST : SCRIPT_ADDRESS, &id, 20);
        return true;
    }

    bool Set(const CTxDestination &dest)
    {
        return boost::apply_visitor(CBitcoinAddressVisitor(this), dest);
    }

    bool IsValid() const
    {
        unsigned int nExpectedSize = 20;
        bool fExpectTestNet = false;
        switch(nVersion)
        {
            case PUBKEY_ADDRESS:
                nExpectedSize = 20; // Hash of public key
                fExpectTestNet = false;
                break;
            case SCRIPT_ADDRESS:
                nExpectedSize = 20; // Hash of CScript
                fExpectTestNet = false;
                break;

            case PUBKEY_ADDRESS_TEST:
                nExpectedSize = 20;
                fExpectTestNet = true;
                break;
            case SCRIPT_ADDRESS_TEST:
                nExpectedSize = 20;
                fExpectTestNet = true;
                break;

            default:
                return false;
        }
        return fExpectTestNet == fTestNet && vchData.size() == nExpectedSize;
    }

    CBitcoinAddress()
    {
    }

    CBitcoinAddress(const CTxDestination &dest)
    {
        Set(dest);
    }

    CBitcoinAddress(const std::string& strAddress)
    {
        SetString(strAddress);
    }

    CBitcoinAddress(const char* pszAddress)
    {
        SetString(pszAddress);
    }

    CTxDestination Get() const {
        if (!IsValid())
            return CNoDestination();
        switch (nVersion) {
        case PUBKEY_ADDRESS:
        case PUBKEY_ADDRESS_TEST: {
            uint160 id;
            memcpy(&id, &vchData[0], 20);
            return CKeyID(id);
        }
        case SCRIPT_ADDRESS:
        case SCRIPT_ADDRESS_TEST: {
            uint160 id;
            memcpy(&id, &vchData[0], 20);
            return CScriptID(id);
        }
        }
        return CNoDestination();
    }

    bool GetKeyID(CKeyID &keyID) const {
        if (!IsValid())
            return false;
        switch (nVersion) {
        case PUBKEY_ADDRESS:
        case PUBKEY_ADDRESS_TEST: {
            uint160 id;
            memcpy(&id, &vchData[0], 20);
            keyID = CKeyID(id);
            return true;
        }
        default: return false;
        }
    }

    bool IsScript() const {
        if (!IsValid())
            return false;
        switch (nVersion) {
        case SCRIPT_ADDRESS:
        case SCRIPT_ADDRESS_TEST: {
            return true;
        }
        default: return false;
        }
    }
};

bool inline CBitcoinAddressVisitor::operator()(const CKeyID &id) const         { return addr->Set(id); }
bool inline CBitcoinAddressVisitor::operator()(const CScriptID &id) const      { return addr->Set(id); }
bool inline CBitcoinAddressVisitor::operator()(const CNoDestination &id) const { return false; }

/** A base58-encoded secret key */
class CBitcoinSecret : public CBase58Data
{
public:
    void SetSecret(const CSecret& vchSecret, bool fCompressed)
    {
        assert(vchSecret.size() == 32);
        SetData(fTestNet ? 239 : 128, &vchSecret[0], vchSecret.size());
        if (fCompressed)
            vchData.push_back(1);
    }

    CSecret GetSecret(bool &fCompressedOut)
    {
        CSecret vchSecret;
        vchSecret.resize(32);
        memcpy(&vchSecret[0], &vchData[0], 32);
        fCompressedOut = vchData.size() == 33;
        return vchSecret;
    }

    bool IsValid() const
    {
        bool fExpectTestNet = false;
        switch(nVersion)
        {
            case 128:
                break;

            case 239:
                fExpectTestNet = true;
                break;

            default:
                return false;
        }
        return fExpectTestNet == fTestNet && (vchData.size() == 32 || (vchData.size() == 33 && vchData[32] == 1));
    }

    bool SetString(const char* pszSecret)
    {
        return CBase58Data::SetString(pszSecret) && IsValid();
    }

    bool SetString(const std::string& strSecret)
    {
        return SetString(strSecret.c_str());
    }

    CBitcoinSecret(const CSecret& vchSecret, bool fCompressed)
    {
        SetSecret(vchSecret, fCompressed);
    }

    CBitcoinSecret()
    {
    }
};

#endif // BITCOIN_BASE58_H
