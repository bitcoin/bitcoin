// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include <assert.h>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>

extern const signed char p_util_hexdigit[256]; // defined in util.cpp

inline signed char HexDigit(char c)
{
    return p_util_hexdigit[(unsigned char)c];
}

class uint_error : public std::runtime_error {
public:
    explicit uint_error(const std::string& str) : std::runtime_error(str) {}
};

/** Template base class for unsigned big integers. */
template<unsigned int BITS>
class base_uint
{
private:
    enum { WIDTH=BITS/32 };
    uint32_t pn[WIDTH];
public:

    base_uint()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    base_uint(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    base_uint& operator=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    base_uint(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    explicit base_uint(const std::string& str)
    {
        SetHex(str);
    }

    explicit base_uint(const std::vector<unsigned char>& vch)
    {
        if (vch.size() != sizeof(pn))
            throw uint_error("Converting vector of wrong size to base_uint");
        memcpy(pn, &vch[0], sizeof(pn));
    }

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

    base_uint& operator*=(uint32_t b32)
    {
        uint64_t carry = 0;
        for (int i = 0; i < WIDTH; i++)
        {
            uint64_t n = carry + (uint64_t)b32 * pn[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_uint& operator*=(const base_uint& b)
    {
        base_uint a = *this;
        *this = 0;
        for (int j = 0; j < WIDTH; j++) {
            uint64_t carry = 0;
            for (int i = 0; i + j < WIDTH; i++) {
                uint64_t n = carry + pn[i + j] + (uint64_t)a.pn[j] * b.pn[i];
                pn[i + j] = n & 0xffffffff;
                carry = n >> 32;
            }
        }
        return *this;
    }

    base_uint& operator/=(const base_uint& b)
    {
        base_uint div = b; // make a copy, so we can shift.
        base_uint num = *this; // make a copy, so we can subtract.
        *this = 0; // the quotient.
        int num_bits = num.bits();
        int div_bits = div.bits();
        if (div_bits == 0)
            throw uint_error("Division by zero");
        if (div_bits > num_bits) // the result is certainly 0.
            return *this;
        int shift = num_bits - div_bits;
        div <<= shift; // shift so that div and nun align.
        while (shift >= 0) {
            if (num >= div) {
                num -= div;
                pn[shift / 32] |= (1 << (shift & 31)); // set a bit of the result.
            }
            div >>= 1; // shift back.
            shift--;
        }
        // num now contains the remainder of the division.
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

    int CompareTo(const base_uint& b) const {
        for (int i = base_uint::WIDTH-1; i >= 0; i--) {
            if (pn[i] < b.pn[i])
                return -1;
            if (pn[i] > b.pn[i])
                return 1;
        }
        return 0;
    }

    bool EqualTo(uint64_t b) const {
        for (int i = base_uint::WIDTH-1; i >= 2; i--) {
            if (pn[i])
                return false;
        }
        if (pn[1] != (b >> 32))
            return false;
        if (pn[0] != (b & 0xfffffffful))
            return false;
        return true;
    }

    friend inline const base_uint operator+(const base_uint& a, const base_uint& b) { return base_uint(a) += b; }
    friend inline const base_uint operator-(const base_uint& a, const base_uint& b) { return base_uint(a) -= b; }
    friend inline const base_uint operator*(const base_uint& a, const base_uint& b) { return base_uint(a) *= b; }
    friend inline const base_uint operator/(const base_uint& a, const base_uint& b) { return base_uint(a) /= b; }
    friend inline const base_uint operator|(const base_uint& a, const base_uint& b) { return base_uint(a) |= b; }
    friend inline const base_uint operator&(const base_uint& a, const base_uint& b) { return base_uint(a) &= b; }
    friend inline const base_uint operator^(const base_uint& a, const base_uint& b) { return base_uint(a) ^= b; }
    friend inline const base_uint operator>>(const base_uint& a, int shift) { return base_uint(a) >>= shift; }
    friend inline const base_uint operator<<(const base_uint& a, int shift) { return base_uint(a) <<= shift; }
    friend inline const base_uint operator*(const base_uint& a, uint32_t b) { return base_uint(a) *= b; }
    friend inline bool operator==(const base_uint& a, const base_uint& b) { return a.CompareTo(b) == 0; }
    friend inline bool operator!=(const base_uint& a, const base_uint& b) { return a.CompareTo(b) != 0; }
    friend inline bool operator>(const base_uint& a, const base_uint& b) { return a.CompareTo(b) > 0; }
    friend inline bool operator<(const base_uint& a, const base_uint& b) { return a.CompareTo(b) < 0; }
    friend inline bool operator>=(const base_uint& a, const base_uint& b) { return a.CompareTo(b) >= 0; }
    friend inline bool operator<=(const base_uint& a, const base_uint& b) { return a.CompareTo(b) <= 0; }
    friend inline bool operator==(const base_uint& a, uint64_t b) { return a.EqualTo(b); }
    friend inline bool operator!=(const base_uint& a, uint64_t b) { return !a.EqualTo(b); }

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

    // Returns the position of the highest bit set plus one, or zero if the
    // value is zero.
    unsigned int bits() const
    {
        for (int pos = WIDTH-1; pos >= 0; pos--) {
            if (pn[pos]) {
                for (int bits = 31; bits > 0; bits--) {
                    if (pn[pos] & 1<<bits)
                        return 32*pos + bits + 1;
                }
                return 32*pos + 1;
            }
        }
        return 0;
    }

    uint64_t GetLow64() const
    {
        assert(WIDTH >= 2);
        return pn[0] | (uint64_t)pn[1] << 32;
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const
    {
        return sizeof(pn);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType, int nVersion) const
    {
        s.write((char*)pn, sizeof(pn));
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        s.read((char*)pn, sizeof(pn));
    }
};

/** 160-bit unsigned big integer. */
class uint160 : public base_uint<160> {
public:
    uint160() {}
    uint160(const base_uint<160>& b) : base_uint<160>(b) {}
    uint160(uint64_t b) : base_uint<160>(b) {}
    explicit uint160(const std::string& str) : base_uint<160>(str) {}
    explicit uint160(const std::vector<unsigned char>& vch) : base_uint<160>(vch) {}
};

/** 256-bit unsigned big integer. */
class uint256 : public base_uint<256> {
public:
    uint256() {}
    uint256(const base_uint<256>& b) : base_uint<256>(b) {}
    uint256(uint64_t b) : base_uint<256>(b) {}
    explicit uint256(const std::string& str) : base_uint<256>(str) {}
    explicit uint256(const std::vector<unsigned char>& vch) : base_uint<256>(vch) {}

    // The "compact" format is a representation of a whole
    // number N using an unsigned 32bit number similar to a
    // floating point format.
    // The most significant 8 bits are the unsigned exponent of base 256.
    // This exponent can be thought of as "number of bytes of N".
    // The lower 23 bits are the mantissa.
    // Bit number 24 (0x800000) represents the sign of N.
    // N = (-1^sign) * mantissa * 256^(exponent-3)
    //
    // Satoshi's original implementation used BN_bn2mpi() and BN_mpi2bn().
    // MPI uses the most significant bit of the first byte as sign.
    // Thus 0x1234560000 is compact (0x05123456)
    // and  0xc0de000000 is compact (0x0600c0de)
    // (0x05c0de00) would be -0x40de000000
    //
    // Bitcoin only uses this "compact" format for encoding difficulty
    // targets, which are unsigned 256bit quantities.  Thus, all the
    // complexities of the sign bit and using base 256 are probably an
    // implementation accident.
    //
    // This implementation directly uses shifts instead of going
    // through an intermediate MPI representation.
    uint256& SetCompact(uint32_t nCompact, bool *pfNegative = NULL, bool *pfOverflow = NULL)
    {
        int nSize = nCompact >> 24;
        uint32_t nWord = nCompact & 0x007fffff;
        if (nSize <= 3)
        {
            nWord >>= 8*(3-nSize);
            *this = nWord;
        }
        else
        {
            *this = nWord;
            *this <<= 8*(nSize-3);
        }
        if (pfNegative)
            *pfNegative = nWord != 0 && (nCompact & 0x00800000) != 0;
        if (pfOverflow)
            *pfOverflow = nWord != 0 && ((nSize > 34) ||
                                         (nWord > 0xff && nSize > 33) ||
                                         (nWord > 0xffff && nSize > 32));
        return *this;
    }

    uint32_t GetCompact(bool fNegative = false) const
    {
        int nSize = (bits() + 7) / 8;
        uint32_t nCompact = 0;
        if (nSize <= 3)
            nCompact = GetLow64() << 8*(3-nSize);
        else
        {
            uint256 bn = *this >> 8*(nSize-3);
            nCompact = bn.GetLow64();
        }
        // The 0x00800000 bit denotes the sign.
        // Thus, if it is already set, divide the mantissa by 256 and increase the exponent.
        if (nCompact & 0x00800000)
        {
            nCompact >>= 8;
            nSize++;
        }
        assert((nCompact & ~0x007fffff) == 0);
        assert(nSize < 256);
        nCompact |= nSize << 24;
        nCompact |= (fNegative && (nCompact & 0x007fffff) ? 0x00800000 : 0);
        return nCompact;
    }
};

#endif
