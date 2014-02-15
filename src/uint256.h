// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include <stdint.h>
#include <stdio.h>
#include <sstream>
#include <string>
#include <string.h>
#include <stdexcept> // for runtime_error
#include <algorithm> // for std::copy
#include <map>
#include <vector>

// defined in util.cpp
extern int ilog2(int value);
extern const signed char p_util_hexdigit[256];
inline signed char HexDigit(char c)
    { return p_util_hexdigit[(unsigned char)c]; }
extern const char *pbase32;
std::string EncodeBase32(const unsigned char* pch, size_t len);
std::vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid);
std::string AddErrorCorrectionCode32(const std::string& str);
std::string RemoveErrorCorrectionCode32(const std::string& str, bool* pfInvalid, std::map<size_t, char> *pMapErrors);
/** Base class without constructors for uint256 and uint160.
 * This makes the compiler let you use it in a union.
 */
template<unsigned int BITS>
class base_uint
{
protected:
    enum { WIDTH=BITS/32 };
    uint32_t pn[WIDTH];
public:

    bool operator!() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (pn[i] != 0)
                return false;
        return true;
    }

    const base_uint operator~() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        return ret;
    }

    const base_uint operator-() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        ret++;
        return ret;
    }

    double getdouble() const
    {
        double ret = 0.0;
        double fact = 1.0;
        for (int i = 0; i < WIDTH; i++) {
            ret += fact * pn[i];
            fact *= 4294967296.0;
        }
        return ret;
    }

    base_uint& operator=(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator^=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] ^= b.pn[i];
        return *this;
    }

    base_uint& operator&=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] &= b.pn[i];
        return *this;
    }

    base_uint& operator|=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] |= b.pn[i];
        return *this;
    }

    base_uint& operator^=(uint64_t b)
    {
        pn[0] ^= (unsigned int)b;
        pn[1] ^= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator|=(uint64_t b)
    {
        pn[0] |= (unsigned int)b;
        pn[1] |= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator<<=(unsigned int shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i+k+1 < WIDTH && shift != 0)
                pn[i+k+1] |= (a.pn[i] >> (32-shift));
            if (i+k < WIDTH)
                pn[i+k] |= (a.pn[i] << shift);
        }
        return *this;
    }

    base_uint& operator>>=(unsigned int shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i-k-1 >= 0 && shift != 0)
                pn[i-k-1] |= (a.pn[i] << (32-shift));
            if (i-k >= 0)
                pn[i-k] |= (a.pn[i] >> shift);
        }
        return *this;
    }

    base_uint& operator+=(const base_uint& b)
    {
        uint64_t carry = 0;
        for (int i = 0; i < WIDTH; i++)
        {
            uint64_t n = carry + pn[i] + b.pn[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_uint& operator-=(const base_uint& b)
    {
        *this += -b;
        return *this;
    }

    base_uint& operator+=(uint64_t b64)
    {
        base_uint b;
        b = b64;
        *this += b;
        return *this;
    }

    base_uint& operator-=(uint64_t b64)
    {
        base_uint b;
        b = b64;
        *this += -b;
        return *this;
    }


    base_uint& operator++()
    {
        // prefix operator
        int i = 0;
        while (++pn[i] == 0 && i < WIDTH-1)
            i++;
        return *this;
    }

    const base_uint operator++(int)
    {
        // postfix operator
        const base_uint ret = *this;
        ++(*this);
        return ret;
    }

    base_uint& operator--()
    {
        // prefix operator
        int i = 0;
        while (--pn[i] == (uint32_t)-1 && i < WIDTH-1)
            i++;
        return *this;
    }

    const base_uint operator--(int)
    {
        // postfix operator
        const base_uint ret = *this;
        --(*this);
        return ret;
    }


    friend inline bool operator<(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator<=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator>(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator>=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator==(const base_uint& a, const base_uint& b)
    {
        for (int i = 0; i < base_uint::WIDTH; i++)
            if (a.pn[i] != b.pn[i])
                return false;
        return true;
    }

    friend inline bool operator==(const base_uint& a, uint64_t b)
    {
        if (a.pn[0] != (unsigned int)b)
            return false;
        if (a.pn[1] != (unsigned int)(b >> 32))
            return false;
        for (int i = 2; i < base_uint::WIDTH; i++)
            if (a.pn[i] != 0)
                return false;
        return true;
    }

    friend inline bool operator!=(const base_uint& a, const base_uint& b)
    {
        return (!(a == b));
    }

    friend inline bool operator!=(const base_uint& a, uint64_t b)
    {
        return (!(a == b));
    }



    std::string GetHex() const
    {
        char psz[sizeof(pn)*2 + 1];
        for (unsigned int i = 0; i < sizeof(pn); i++)
            sprintf(psz + i*2, "%02x", ((unsigned char*)pn)[sizeof(pn) - i - 1]);
        return std::string(psz, psz + sizeof(pn)*2);
    }

    void SetHex(const char* psz)
    {
        memset(pn,0,sizeof(pn));

        // skip leading spaces
        while (isspace(*psz))
            psz++;

        // skip 0x
        if (psz[0] == '0' && tolower(psz[1]) == 'x')
            psz += 2;

        // hex string to uint
        const char* pbegin = psz;
        while (::HexDigit(*psz) != -1)
            psz++;
        psz--;
        unsigned char* p1 = (unsigned char*)pn;
        unsigned char* pend = p1 + WIDTH * 4;
        while (psz >= pbegin && p1 < pend)
        {
            *p1 = ::HexDigit(*psz--);
            if (psz >= pbegin)
            {
                *p1 |= ((unsigned char)::HexDigit(*psz--) << 4);
                p1++;
            }
        }
    }

    void SetHex(const std::string& str)
    {
        SetHex(str.c_str());
    }

    std::string ToString() const
    {
        return (GetHex());
    }

    std::string GetCodedBase32(int nExtra=0) const
    {
        // The length of the decoded payload is always a power-of-2 multiple of 128.
        // The parameter e is the solution to the equations size=128*n and n=2^e.
        // Most or all of the following bit-twiddling should be optimized away by any decent compiler.
        // See: <http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2>
        int n, e, i;
        e = (BITS+127) / 128 - 1;
        e = (e >> 1)  | e;
        e = (e >> 2)  | e;
        e = (e >> 4)  | e;
        e = (e >> 8)  | e;
        e = (e >> 16) | e;
        e = ilog2(e + 1);
        n = 1 << e;

        std::vector<unsigned char> payload(
            reinterpret_cast<const unsigned char*>(pn),
            reinterpret_cast<const unsigned char*>(pn)+BITS/8);
        payload.resize((130*n+7)/8); // 130 bits per 128 bits input

        std::vector<unsigned char>::iterator begin = payload.begin();
        std::vector<unsigned char>::iterator end   = payload.begin()+16*n;
        std::vector<unsigned char>::iterator itr;

        // Shift in 2*n zeroed padding bits to form the payload prefix.
        div_t d = div(2*n, 8);
        if (d.rem)
        {
            for (itr = end++; itr != begin; --itr)
                *itr = *itr >> d.rem | (*(itr-1) & (1<<d.rem)-1) << 8-d.rem;
            *begin >>= d.rem;
        }
        if (d.quot)
        {
            std::copy_backward(begin, end, payload.end());
            std::fill(begin, begin + d.quot, 0);
        }

        // Add prefix bits encoding the length of the payload.
        for (i=0; i<e; ++i)
            *(begin + i/8) |= 1 << (7 - (i&7));

        // Add the nExtra field, which is 2*n-e-1 bits in length
        for (i=0; i<2*n-e-1; ++i, nExtra>>=1)
        {
            d = div(2*n - i - 1, 8);
            *(begin + d.quot) |= (nExtra&1) << (7 - d.rem);
        }

        std::string b32 = EncodeBase32(&payload[0], payload.size());
        b32.resize((BITS+2*n+4)/5); // strip redundant padding

        // Add error correction encoding, and return.
        return AddErrorCorrectionCode32(b32);
    }

    void SetCodedBase32(const char* psz, int *pnExtra=NULL)
    {
        // The length of the decoded payload is always a power-of-2 multiple of 128.
        // The parameter e is the solution to the equations size=128*n and n=2^e.
        // Most or all of the following bit-twiddling should be optimized away by any decent compiler.
        // See: <http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2>
        int n, e, i;
        e = (BITS+127) / 128 - 1;
        e = (e >> 1)  | e;
        e = (e >> 2)  | e;
        e = (e >> 4)  | e;
        e = (e >> 8)  | e;
        e = (e >> 16) | e;
        e = ilog2(e + 1);
        n = 1 << e;

        std::string b32(psz);

        // Drop prefix characters (e.g. "tx") if present
        div_t d = div(BITS+2*n, 130);
        size_t len = static_cast<size_t>(
            31*d.quot + (d.rem+4)/5 + 5*((d.rem+129)/130));
        if (b32.size() < len)
        {
            std::ostringstream ss;
            ss << "improper length: expected " << len
               << " z-base-32 digits, received " << b32.size()
               << " quot=" << d.quot << ",rem=" << d.rem;
            throw std::runtime_error(ss.str());
        }
        if (b32.size() > len)
            b32 = b32.substr(b32.size()-len, std::string::npos);

        std::map<size_t, char> mapErrors;
        b32 = RemoveErrorCorrectionCode32(b32, NULL, &mapErrors);

        // Add zero-valued padding to achieve 8-bit alignment
        b32.resize((b32.size()+7)&~7, pbase32[0]);
        std::vector<unsigned char> payload = DecodeBase32(b32.c_str(), NULL);

        std::vector<unsigned char>::iterator begin = payload.begin();
        std::vector<unsigned char>::iterator itr;

        // Check prefix bits encoding the length of the payload.
        for (i=0; i<e; ++i)
        {
            if (!(*(begin + i/8) & 1 << (7 - (i&7))))
            {
                std::ostringstream ss;
                ss << "invalid padding: prefix[" << i << "] bit is 0, instead of 1";
                throw std::runtime_error(ss.str());
            }
        }
        if (*(begin + i/8) & 1 << (7 - (i&7)))
        {
            std::ostringstream ss;
            ss << "invalid padding: prefix[" << i << "] bit is 1, instead of 0";
            throw std::runtime_error(ss.str());
        }

        // Extract nExtra field, which is 2*n-e-1 bits in length.
        if (pnExtra) {
            *pnExtra = 0;
            for (++i; i < 2*n; ++i) {
                *pnExtra <<= 1;
                if (*(begin + i/8) & (1 << (7 - (i&7))))
                    *pnExtra |= 1;
            }
        }

        // Shift out the 2*n padding bits from the payload prefix.
        d = div(2*n, 8);
        if (d.quot)
            std::copy(payload.begin()+d.quot,
                      payload.end(),
                      payload.begin());
        if (d.rem)
            for (itr=begin; itr != begin+16*n; ++itr)
                *itr = *itr << d.rem | (*(itr+1) >> 8-d.rem) & (1<<d.rem)-1;

        std::copy(&payload[0],
                  &payload[BITS/8],
                  reinterpret_cast<unsigned char*>(pn));
    }

    void SetCodedBase32(const std::string& str, int *pnExtra=NULL)
    {
        SetCodedBase32(str.c_str(), pnExtra);
    }

    unsigned char* begin()
    {
        return (unsigned char*)&pn[0];
    }

    unsigned char* end()
    {
        return (unsigned char*)&pn[WIDTH];
    }

    const unsigned char* begin() const
    {
        return (unsigned char*)&pn[0];
    }

    const unsigned char* end() const
    {
        return (unsigned char*)&pn[WIDTH];
    }

    unsigned int size() const
    {
        return sizeof(pn);
    }

    uint64_t GetLow64() const
    {
        assert(WIDTH >= 2);
        return pn[0] | (uint64_t)pn[1] << 32;
    }

//    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    unsigned int GetSerializeSize(int nType, int nVersion) const
    {
        return sizeof(pn);
    }

    template<typename Stream>
//    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    void Serialize(Stream& s, int nType, int nVersion) const
    {
        s.write((char*)pn, sizeof(pn));
    }

    template<typename Stream>
//    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        s.read((char*)pn, sizeof(pn));
    }


    friend class uint160;
    friend class uint256;
};

typedef base_uint<160> base_uint160;
typedef base_uint<256> base_uint256;



//
// uint160 and uint256 could be implemented as templates, but to keep
// compile errors and debugging cleaner, they're copy and pasted.
//



//////////////////////////////////////////////////////////////////////////////
//
// uint160
//

/** 160-bit unsigned integer */
class uint160 : public base_uint160
{
public:
    typedef base_uint160 basetype;

    uint160()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint160(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint160& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint160(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint160& operator=(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint160(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint160(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint160& a, uint64_t b)                         { return (base_uint160)a == b; }
inline bool operator!=(const uint160& a, uint64_t b)                         { return (base_uint160)a != b; }
inline const uint160 operator<<(const base_uint160& a, unsigned int shift)   { return uint160(a) <<= shift; }
inline const uint160 operator>>(const base_uint160& a, unsigned int shift)   { return uint160(a) >>= shift; }
inline const uint160 operator<<(const uint160& a, unsigned int shift)        { return uint160(a) <<= shift; }
inline const uint160 operator>>(const uint160& a, unsigned int shift)        { return uint160(a) >>= shift; }

inline const uint160 operator^(const base_uint160& a, const base_uint160& b) { return uint160(a) ^= b; }
inline const uint160 operator&(const base_uint160& a, const base_uint160& b) { return uint160(a) &= b; }
inline const uint160 operator|(const base_uint160& a, const base_uint160& b) { return uint160(a) |= b; }
inline const uint160 operator+(const base_uint160& a, const base_uint160& b) { return uint160(a) += b; }
inline const uint160 operator-(const base_uint160& a, const base_uint160& b) { return uint160(a) -= b; }

inline bool operator<(const base_uint160& a, const uint160& b)               { return (base_uint160)a <  (base_uint160)b; }
inline bool operator<=(const base_uint160& a, const uint160& b)              { return (base_uint160)a <= (base_uint160)b; }
inline bool operator>(const base_uint160& a, const uint160& b)               { return (base_uint160)a >  (base_uint160)b; }
inline bool operator>=(const base_uint160& a, const uint160& b)              { return (base_uint160)a >= (base_uint160)b; }
inline bool operator==(const base_uint160& a, const uint160& b)              { return (base_uint160)a == (base_uint160)b; }
inline bool operator!=(const base_uint160& a, const uint160& b)              { return (base_uint160)a != (base_uint160)b; }
inline const uint160 operator^(const base_uint160& a, const uint160& b)      { return (base_uint160)a ^  (base_uint160)b; }
inline const uint160 operator&(const base_uint160& a, const uint160& b)      { return (base_uint160)a &  (base_uint160)b; }
inline const uint160 operator|(const base_uint160& a, const uint160& b)      { return (base_uint160)a |  (base_uint160)b; }
inline const uint160 operator+(const base_uint160& a, const uint160& b)      { return (base_uint160)a +  (base_uint160)b; }
inline const uint160 operator-(const base_uint160& a, const uint160& b)      { return (base_uint160)a -  (base_uint160)b; }

inline bool operator<(const uint160& a, const base_uint160& b)               { return (base_uint160)a <  (base_uint160)b; }
inline bool operator<=(const uint160& a, const base_uint160& b)              { return (base_uint160)a <= (base_uint160)b; }
inline bool operator>(const uint160& a, const base_uint160& b)               { return (base_uint160)a >  (base_uint160)b; }
inline bool operator>=(const uint160& a, const base_uint160& b)              { return (base_uint160)a >= (base_uint160)b; }
inline bool operator==(const uint160& a, const base_uint160& b)              { return (base_uint160)a == (base_uint160)b; }
inline bool operator!=(const uint160& a, const base_uint160& b)              { return (base_uint160)a != (base_uint160)b; }
inline const uint160 operator^(const uint160& a, const base_uint160& b)      { return (base_uint160)a ^  (base_uint160)b; }
inline const uint160 operator&(const uint160& a, const base_uint160& b)      { return (base_uint160)a &  (base_uint160)b; }
inline const uint160 operator|(const uint160& a, const base_uint160& b)      { return (base_uint160)a |  (base_uint160)b; }
inline const uint160 operator+(const uint160& a, const base_uint160& b)      { return (base_uint160)a +  (base_uint160)b; }
inline const uint160 operator-(const uint160& a, const base_uint160& b)      { return (base_uint160)a -  (base_uint160)b; }

inline bool operator<(const uint160& a, const uint160& b)                    { return (base_uint160)a <  (base_uint160)b; }
inline bool operator<=(const uint160& a, const uint160& b)                   { return (base_uint160)a <= (base_uint160)b; }
inline bool operator>(const uint160& a, const uint160& b)                    { return (base_uint160)a >  (base_uint160)b; }
inline bool operator>=(const uint160& a, const uint160& b)                   { return (base_uint160)a >= (base_uint160)b; }
inline bool operator==(const uint160& a, const uint160& b)                   { return (base_uint160)a == (base_uint160)b; }
inline bool operator!=(const uint160& a, const uint160& b)                   { return (base_uint160)a != (base_uint160)b; }
inline const uint160 operator^(const uint160& a, const uint160& b)           { return (base_uint160)a ^  (base_uint160)b; }
inline const uint160 operator&(const uint160& a, const uint160& b)           { return (base_uint160)a &  (base_uint160)b; }
inline const uint160 operator|(const uint160& a, const uint160& b)           { return (base_uint160)a |  (base_uint160)b; }
inline const uint160 operator+(const uint160& a, const uint160& b)           { return (base_uint160)a +  (base_uint160)b; }
inline const uint160 operator-(const uint160& a, const uint160& b)           { return (base_uint160)a -  (base_uint160)b; }



//////////////////////////////////////////////////////////////////////////////
//
// uint256
//

/** 256-bit unsigned integer */
class uint256 : public base_uint256
{
public:
    typedef base_uint256 basetype;

    uint256()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint256& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint256(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256& operator=(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint256(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint256(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint256& a, uint64_t b)                          { return (base_uint256)a == b; }
inline bool operator!=(const uint256& a, uint64_t b)                          { return (base_uint256)a != b; }
inline const uint256 operator<<(const base_uint256& a, unsigned int shift)   { return uint256(a) <<= shift; }
inline const uint256 operator>>(const base_uint256& a, unsigned int shift)   { return uint256(a) >>= shift; }
inline const uint256 operator<<(const uint256& a, unsigned int shift)        { return uint256(a) <<= shift; }
inline const uint256 operator>>(const uint256& a, unsigned int shift)        { return uint256(a) >>= shift; }

inline const uint256 operator^(const base_uint256& a, const base_uint256& b) { return uint256(a) ^= b; }
inline const uint256 operator&(const base_uint256& a, const base_uint256& b) { return uint256(a) &= b; }
inline const uint256 operator|(const base_uint256& a, const base_uint256& b) { return uint256(a) |= b; }
inline const uint256 operator+(const base_uint256& a, const base_uint256& b) { return uint256(a) += b; }
inline const uint256 operator-(const base_uint256& a, const base_uint256& b) { return uint256(a) -= b; }

inline bool operator<(const base_uint256& a, const uint256& b)          { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const base_uint256& a, const uint256& b)         { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const base_uint256& a, const uint256& b)          { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const base_uint256& a, const uint256& b)         { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const base_uint256& a, const uint256& b)         { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const base_uint256& a, const uint256& b)         { return (base_uint256)a != (base_uint256)b; }
inline const uint256 operator^(const base_uint256& a, const uint256& b) { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256 operator&(const base_uint256& a, const uint256& b) { return (base_uint256)a &  (base_uint256)b; }
inline const uint256 operator|(const base_uint256& a, const uint256& b) { return (base_uint256)a |  (base_uint256)b; }
inline const uint256 operator+(const base_uint256& a, const uint256& b) { return (base_uint256)a +  (base_uint256)b; }
inline const uint256 operator-(const base_uint256& a, const uint256& b) { return (base_uint256)a -  (base_uint256)b; }

inline bool operator<(const uint256& a, const base_uint256& b)          { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const uint256& a, const base_uint256& b)         { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const uint256& a, const base_uint256& b)          { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const uint256& a, const base_uint256& b)         { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const uint256& a, const base_uint256& b)         { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const uint256& a, const base_uint256& b)         { return (base_uint256)a != (base_uint256)b; }
inline const uint256 operator^(const uint256& a, const base_uint256& b) { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256 operator&(const uint256& a, const base_uint256& b) { return (base_uint256)a &  (base_uint256)b; }
inline const uint256 operator|(const uint256& a, const base_uint256& b) { return (base_uint256)a |  (base_uint256)b; }
inline const uint256 operator+(const uint256& a, const base_uint256& b) { return (base_uint256)a +  (base_uint256)b; }
inline const uint256 operator-(const uint256& a, const base_uint256& b) { return (base_uint256)a -  (base_uint256)b; }

inline bool operator<(const uint256& a, const uint256& b)               { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const uint256& a, const uint256& b)              { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const uint256& a, const uint256& b)               { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const uint256& a, const uint256& b)              { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const uint256& a, const uint256& b)              { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const uint256& a, const uint256& b)              { return (base_uint256)a != (base_uint256)b; }
inline const uint256 operator^(const uint256& a, const uint256& b)      { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256 operator&(const uint256& a, const uint256& b)      { return (base_uint256)a &  (base_uint256)b; }
inline const uint256 operator|(const uint256& a, const uint256& b)      { return (base_uint256)a |  (base_uint256)b; }
inline const uint256 operator+(const uint256& a, const uint256& b)      { return (base_uint256)a +  (base_uint256)b; }
inline const uint256 operator-(const uint256& a, const uint256& b)      { return (base_uint256)a -  (base_uint256)b; }

#endif
