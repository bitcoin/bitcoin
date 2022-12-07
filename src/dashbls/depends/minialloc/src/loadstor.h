//
// This file contains code from randombit/botan library,
// originally released under the Simplified BSD License
// (see COPYING.BSD)
//
// Copyright (c) 1999 - 2022 The Botan Authors
//               1999 - 2017 Jack Lloyd
//                      2007 Yves Jerschow
//

#ifndef MINIALLOC_LOADSTOR_H
#define MINIALLOC_LOADSTOR_H

#include <cstddef>
#include <cstdint>
#include <cstring>

template<size_t B, typename T> inline constexpr uint8_t get_byte(T input)
{
   static_assert(B < sizeof(T), "Valid byte offset");

   const size_t shift = ((~B) & (sizeof(T) - 1)) << 3;
   return static_cast<uint8_t>((input >> shift) & 0xFF);
}

inline constexpr void store_le(uint64_t in, uint8_t out[8])
{
   out[0] = get_byte<7>(in);
   out[1] = get_byte<6>(in);
   out[2] = get_byte<5>(in);
   out[3] = get_byte<4>(in);
   out[4] = get_byte<3>(in);
   out[5] = get_byte<2>(in);
   out[6] = get_byte<1>(in);
   out[7] = get_byte<0>(in);
}

template<typename T>
inline constexpr T load_le(const uint8_t in[], size_t off)
{
    in += off * sizeof(T);
    T out = 0;
    for(size_t i = 0; i != sizeof(T); ++i)
        out = (out << 8) | in[sizeof(T)-1-i];
    return out;
}

#endif // MINIALLOC_LOADSTOR_H
