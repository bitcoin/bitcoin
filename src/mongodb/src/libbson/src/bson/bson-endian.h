/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef BSON_ENDIAN_H
#define BSON_ENDIAN_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#if defined(__sun)
#include <sys/byteorder.h>
#endif

#include "bson-config.h"
#include "bson-macros.h"
#include "bson-compat.h"


BSON_BEGIN_DECLS


#define BSON_BIG_ENDIAN 4321
#define BSON_LITTLE_ENDIAN 1234


#if defined(__sun)
#define BSON_UINT16_SWAP_LE_BE(v) BSWAP_16 ((uint16_t) v)
#define BSON_UINT32_SWAP_LE_BE(v) BSWAP_32 ((uint32_t) v)
#define BSON_UINT64_SWAP_LE_BE(v) BSWAP_64 ((uint64_t) v)
#elif defined(__clang__) && defined(__clang_major__) &&  \
   defined(__clang_minor__) && (__clang_major__ >= 3) && \
   (__clang_minor__ >= 1)
#if __has_builtin(__builtin_bswap16)
#define BSON_UINT16_SWAP_LE_BE(v) __builtin_bswap16 (v)
#endif
#if __has_builtin(__builtin_bswap32)
#define BSON_UINT32_SWAP_LE_BE(v) __builtin_bswap32 (v)
#endif
#if __has_builtin(__builtin_bswap64)
#define BSON_UINT64_SWAP_LE_BE(v) __builtin_bswap64 (v)
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#if __GNUC__ > 4 || (defined(__GNUC_MINOR__) && __GNUC_MINOR__ >= 3)
#define BSON_UINT32_SWAP_LE_BE(v) __builtin_bswap32 ((uint32_t) v)
#define BSON_UINT64_SWAP_LE_BE(v) __builtin_bswap64 ((uint64_t) v)
#endif
#if __GNUC__ > 4 || (defined(__GNUC_MINOR__) && __GNUC_MINOR__ >= 8)
#define BSON_UINT16_SWAP_LE_BE(v) __builtin_bswap16 ((uint32_t) v)
#endif
#endif


#ifndef BSON_UINT16_SWAP_LE_BE
#define BSON_UINT16_SWAP_LE_BE(v) __bson_uint16_swap_slow ((uint16_t) v)
#endif


#ifndef BSON_UINT32_SWAP_LE_BE
#define BSON_UINT32_SWAP_LE_BE(v) __bson_uint32_swap_slow ((uint32_t) v)
#endif


#ifndef BSON_UINT64_SWAP_LE_BE
#define BSON_UINT64_SWAP_LE_BE(v) __bson_uint64_swap_slow ((uint64_t) v)
#endif


#if BSON_BYTE_ORDER == BSON_LITTLE_ENDIAN
#define BSON_UINT16_FROM_LE(v) ((uint16_t) v)
#define BSON_UINT16_TO_LE(v) ((uint16_t) v)
#define BSON_UINT16_FROM_BE(v) BSON_UINT16_SWAP_LE_BE (v)
#define BSON_UINT16_TO_BE(v) BSON_UINT16_SWAP_LE_BE (v)
#define BSON_UINT32_FROM_LE(v) ((uint32_t) v)
#define BSON_UINT32_TO_LE(v) ((uint32_t) v)
#define BSON_UINT32_FROM_BE(v) BSON_UINT32_SWAP_LE_BE (v)
#define BSON_UINT32_TO_BE(v) BSON_UINT32_SWAP_LE_BE (v)
#define BSON_UINT64_FROM_LE(v) ((uint64_t) v)
#define BSON_UINT64_TO_LE(v) ((uint64_t) v)
#define BSON_UINT64_FROM_BE(v) BSON_UINT64_SWAP_LE_BE (v)
#define BSON_UINT64_TO_BE(v) BSON_UINT64_SWAP_LE_BE (v)
#define BSON_DOUBLE_FROM_LE(v) ((double) v)
#define BSON_DOUBLE_TO_LE(v) ((double) v)
#elif BSON_BYTE_ORDER == BSON_BIG_ENDIAN
#define BSON_UINT16_FROM_LE(v) BSON_UINT16_SWAP_LE_BE (v)
#define BSON_UINT16_TO_LE(v) BSON_UINT16_SWAP_LE_BE (v)
#define BSON_UINT16_FROM_BE(v) ((uint16_t) v)
#define BSON_UINT16_TO_BE(v) ((uint16_t) v)
#define BSON_UINT32_FROM_LE(v) BSON_UINT32_SWAP_LE_BE (v)
#define BSON_UINT32_TO_LE(v) BSON_UINT32_SWAP_LE_BE (v)
#define BSON_UINT32_FROM_BE(v) ((uint32_t) v)
#define BSON_UINT32_TO_BE(v) ((uint32_t) v)
#define BSON_UINT64_FROM_LE(v) BSON_UINT64_SWAP_LE_BE (v)
#define BSON_UINT64_TO_LE(v) BSON_UINT64_SWAP_LE_BE (v)
#define BSON_UINT64_FROM_BE(v) ((uint64_t) v)
#define BSON_UINT64_TO_BE(v) ((uint64_t) v)
#define BSON_DOUBLE_FROM_LE(v) (__bson_double_swap_slow (v))
#define BSON_DOUBLE_TO_LE(v) (__bson_double_swap_slow (v))
#else
#error "The endianness of target architecture is unknown."
#endif


/*
 *--------------------------------------------------------------------------
 *
 * __bson_uint16_swap_slow --
 *
 *       Fallback endianness conversion for 16-bit integers.
 *
 * Returns:
 *       The endian swapped version.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static BSON_INLINE uint16_t
__bson_uint16_swap_slow (uint16_t v) /* IN */
{
   return ((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8);
}


/*
 *--------------------------------------------------------------------------
 *
 * __bson_uint32_swap_slow --
 *
 *       Fallback endianness conversion for 32-bit integers.
 *
 * Returns:
 *       The endian swapped version.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static BSON_INLINE uint32_t
__bson_uint32_swap_slow (uint32_t v) /* IN */
{
   return ((v & 0x000000FFU) << 24) | ((v & 0x0000FF00U) << 8) |
          ((v & 0x00FF0000U) >> 8) | ((v & 0xFF000000U) >> 24);
}


/*
 *--------------------------------------------------------------------------
 *
 * __bson_uint64_swap_slow --
 *
 *       Fallback endianness conversion for 64-bit integers.
 *
 * Returns:
 *       The endian swapped version.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static BSON_INLINE uint64_t
__bson_uint64_swap_slow (uint64_t v) /* IN */
{
   return ((v & 0x00000000000000FFULL) << 56) |
          ((v & 0x000000000000FF00ULL) << 40) |
          ((v & 0x0000000000FF0000ULL) << 24) |
          ((v & 0x00000000FF000000ULL) << 8) |
          ((v & 0x000000FF00000000ULL) >> 8) |
          ((v & 0x0000FF0000000000ULL) >> 24) |
          ((v & 0x00FF000000000000ULL) >> 40) |
          ((v & 0xFF00000000000000ULL) >> 56);
}


/*
 *--------------------------------------------------------------------------
 *
 * __bson_double_swap_slow --
 *
 *       Fallback endianness conversion for double floating point.
 *
 * Returns:
 *       The endian swapped version.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

BSON_STATIC_ASSERT (sizeof (double) == sizeof (uint64_t));

static BSON_INLINE double
__bson_double_swap_slow (double v) /* IN */
{
   uint64_t uv;

   memcpy (&uv, &v, sizeof (v));
   uv = BSON_UINT64_SWAP_LE_BE (uv);
   memcpy (&v, &uv, sizeof (v));

   return v;
}

BSON_END_DECLS


#endif /* BSON_ENDIAN_H */
