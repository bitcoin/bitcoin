// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base_uint.h"
#include <stdio.h>
#include <string.h>
#include "utilstrencodings.h"
#include "crypto/common.h"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
// arith
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
template <unsigned int BITS>
base_uint<BITS>::base_uint(const std::string& str)
{
    SetHex(str);
}

template <unsigned int BITS>
base_uint<BITS>& base_uint<BITS>::operator<<=(unsigned int shift)
{
    base_uint<BITS> a(*this);
    for (int i = 0; i < PN_WIDTH; i++)
        pn[i] = 0;
    int k = shift / 32;
    shift = shift % 32;
    for (int i = 0; i < PN_WIDTH; i++) {
        if (i + k + 1 < PN_WIDTH && shift != 0)
            pn[i + k + 1] |= (a.pn[i] >> (32 - shift));
        if (i + k < PN_WIDTH)
            pn[i + k] |= (a.pn[i] << shift);
    }
    return *this;
}

template <unsigned int BITS>
base_uint<BITS>& base_uint<BITS>::operator>>=(unsigned int shift)
{
    base_uint<BITS> a(*this);
    for (int i = 0; i < PN_WIDTH; i++)
        pn[i] = 0;
    int k = shift / 32;
    shift = shift % 32;
    for (int i = 0; i < PN_WIDTH; i++) {
        if (i - k - 1 >= 0 && shift != 0)
            pn[i - k - 1] |= (a.pn[i] << (32 - shift));
        if (i - k >= 0)
            pn[i - k] |= (a.pn[i] >> shift);
    }
    return *this;
}

template <unsigned int BITS>
base_uint<BITS>& base_uint<BITS>::operator*=(uint32_t b32)
{
    uint64_t carry = 0;
    for (int i = 0; i < PN_WIDTH; i++) {
        uint64_t n = carry + (uint64_t)b32 * pn[i];
        pn[i] = n & 0xffffffff;
        carry = n >> 32;
    }
    return *this;
}

template <unsigned int BITS>
base_uint<BITS>& base_uint<BITS>::operator*=(const base_uint& b)
{
    base_uint<BITS> a = *this;
    *this = 0;
    for (int j = 0; j < PN_WIDTH; j++) {
        uint64_t carry = 0;
        for (int i = 0; i + j < PN_WIDTH; i++) {
            uint64_t n = carry + pn[i + j] + (uint64_t)a.pn[j] * b.pn[i];
            pn[i + j] = n & 0xffffffff;
            carry = n >> 32;
        }
    }
    return *this;
}

template <unsigned int BITS>
base_uint<BITS>& base_uint<BITS>::operator/=(const base_uint& b)
{
    base_uint<BITS> div = b;     // make a copy, so we can shift.
    base_uint<BITS> num = *this; // make a copy, so we can subtract.
    *this = 0;                   // the quotient.
    int num_bits = num.bits();
    int div_bits = div.bits();
    if (div_bits == 0)
        throw uint_error("Division by zero");
    if (div_bits > num_bits) // the result is certainly 0.
        return *this;
    int shift = num_bits - div_bits;
    div <<= shift; // shift so that div and num align.
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

template <unsigned int BITS>
int base_uint<BITS>::CompareTo(const base_uint<BITS>& b) const
{
    for (int i = PN_WIDTH - 1; i >= 0; i--) {
        if (pn[i] < b.pn[i])
            return -1;
        if (pn[i] > b.pn[i])
            return 1;
    }
    return 0;
}

template <unsigned int BITS>
bool base_uint<BITS>::EqualTo(uint64_t b) const
{
    for (int i = PN_WIDTH - 1; i >= 2; i--) {
        if (pn[i])
            return false;
    }
    if (pn[1] != (b >> 32))
        return false;
    if (pn[0] != (b & 0xfffffffful))
        return false;
    return true;
}

template <unsigned int BITS>
double base_uint<BITS>::getdouble() const
{
    double ret = 0.0;
    double fact = 1.0;
    for (int i = 0; i < PN_WIDTH; i++) {
        ret += fact * pn[i];
        fact *= 4294967296.0;
    }
    return ret;
}

template <unsigned int BITS>
unsigned int base_uint<BITS>::bits() const
{
    for (int pos = PN_WIDTH - 1; pos >= 0; pos--) {
        if (pn[pos]) {
            for (int bits = 31; bits > 0; bits--) {
                if (pn[pos] & 1 << bits)
                    return 32 * pos + bits + 1;
            }
            return 32 * pos + 1;
        }
    }
    return 0;
}

// Explicit instantiations for base_uint<160>
template base_uint<160>::base_uint(const std::string&);
template base_uint<160>& base_uint<160>::operator<<=(unsigned int);
template base_uint<160>& base_uint<160>::operator>>=(unsigned int);
template base_uint<160>& base_uint<160>::operator*=(uint32_t b32);
template base_uint<160>& base_uint<160>::operator*=(const base_uint<160>& b);
template base_uint<160>& base_uint<160>::operator/=(const base_uint<160>& b);
template int base_uint<160>::CompareTo(const base_uint<160>&) const;
template bool base_uint<160>::EqualTo(uint64_t) const;
template double base_uint<160>::getdouble() const;
template std::string base_uint<160>::GetHex() const;
template std::string base_uint<160>::ToHexString() const;
template void base_uint<160>::SetHex(const char*);
template void base_uint<160>::SetHex(const std::string&);
template unsigned int base_uint<160>::bits() const;

// Explicit instantiations for base_uint<256>
template base_uint<256>::base_uint(const std::string&);
template base_uint<256>& base_uint<256>::operator<<=(unsigned int);
template base_uint<256>& base_uint<256>::operator>>=(unsigned int);
template base_uint<256>& base_uint<256>::operator*=(uint32_t b32);
template base_uint<256>& base_uint<256>::operator*=(const base_uint<256>& b);
template base_uint<256>& base_uint<256>::operator/=(const base_uint<256>& b);
template int base_uint<256>::CompareTo(const base_uint<256>&) const;
template bool base_uint<256>::EqualTo(uint64_t) const;
template double base_uint<256>::getdouble() const;
template std::string base_uint<256>::GetHex() const;
template std::string base_uint<256>::ToHexString() const;
template void base_uint<256>::SetHex(const char*);
template void base_uint<256>::SetHex(const std::string&);
template unsigned int base_uint<256>::bits() const;


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
// base_blob
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
template <unsigned int BITS>
base_uint<BITS>::base_uint(const std::vector<unsigned char>& vch)
{
    assert(vch.size() == sizeof(pn));
    memcpy(pn, &vch[0], sizeof(pn));
}

template <unsigned int BITS>
std::string base_uint<BITS>::GetHex() const
{
    const unsigned char* pdata = reinterpret_cast<const unsigned char*>(pn);

    char psz[sizeof(pn) * 2 + 1];
    for (unsigned int i = 0; i < sizeof(pn); i++)
        sprintf(psz + i * 2, "%02x", pdata[sizeof(pn) - i - 1]);
    return std::string(psz, psz + sizeof(pn) * 2);
}

template <unsigned int BITS>
void base_uint<BITS>::SetHex(const char* psz)
{
    memset(pn, 0, sizeof(pn));

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
    unsigned char* pend = p1 + BYTE_WIDTH;
    while (psz >= pbegin && p1 < pend) {
        *p1 = ::HexDigit(*psz--);
        if (psz >= pbegin) {
            *p1 |= ((unsigned char)::HexDigit(*psz--) << 4);
            p1++;
        }
    }
}

// Explicit instantiations for base_uint<160>
template base_uint<160>::base_uint(const std::vector<unsigned char>&);
template std::string base_uint<160>::GetHex() const;
template std::string base_uint<160>::ToHexString() const;
template void base_uint<160>::SetHex(const char*);
template void base_uint<160>::SetHex(const std::string&);

// Explicit instantiations for base_uint<256>
template base_uint<256>::base_uint(const std::vector<unsigned char>&);
template std::string base_uint<256>::GetHex() const;
template std::string base_uint<256>::ToHexString() const;
template void base_uint<256>::SetHex(const char*);
template void base_uint<256>::SetHex(const std::string&);
