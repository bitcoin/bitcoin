// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <uint256.h>

template <unsigned int BITS>
std::string base_blob<BITS>::GetHex() const
{
    uint8_t m_data_rev[WIDTH];
    for (int i = 0; i < WIDTH; ++i) {
        m_data_rev[i] = m_data[WIDTH - 1 - i];
    }
    return HexStr(m_data_rev);
}

template <unsigned int BITS>
void base_blob<BITS>::SetHex(std::string_view str)
{
    std::fill(m_data.begin(), m_data.end(), 0);

    str = util::LeftTrimStringView(str);
    str = util::RemovePrefixView(str, "0x");

    // hex string to uint
    size_t digits = 0;
    for (const char c : str) {
        if (::HexDigit(c) == -1)
            break;
        ++digits;
    }
    unsigned char* p1 = m_data.data();
    unsigned char* pend = p1 + WIDTH;
    while (digits > 0 && p1 < pend) {
        *p1 = ::HexDigit(str[--digits]);
        if (digits > 0) {
            *p1 |= ((unsigned char)::HexDigit(str[--digits]) << 4);
            p1++;
        }
    }
}

template <unsigned int BITS>
std::string base_blob<BITS>::ToString() const
{
    return (GetHex());
}

// Explicit instantiations for base_blob<160>
template std::string base_blob<160>::GetHex() const;
template std::string base_blob<160>::ToString() const;
template void base_blob<160>::SetHex(std::string_view);

// Explicit instantiations for base_blob<256>
template std::string base_blob<256>::GetHex() const;
template std::string base_blob<256>::ToString() const;
template void base_blob<256>::SetHex(std::string_view);

const uint256 uint256::ZERO(0);
const uint256 uint256::ONE(1);
