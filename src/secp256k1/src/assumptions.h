/***********************************************************************
 * Copyright (c) 2020 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ASSUMPTIONS_H
#define SECP256K1_ASSUMPTIONS_H

#include <limits.h>

#include "util.h"
#if defined(SECP256K1_INT128_NATIVE)
#include "int128_native.h"
#endif

/* This library, like most software, relies on a number of compiler implementation defined (but not undefined)
   behaviours. Although the behaviours we require are essentially universal we test them specifically here to
   reduce the odds of experiencing an unwelcome surprise.
*/

#if defined(__has_attribute)
# if __has_attribute(__unavailable__)
__attribute__((__unavailable__("Don't call this function. It only exists because STATIC_ASSERT cannot be used outside a function.")))
# endif
#endif
static void secp256k1_assumption_checker(void) {
    /* Bytes are 8 bits. */
    STATIC_ASSERT(CHAR_BIT == 8);

    /* No integer promotion for uint32_t. This ensures that we can multiply uintXX_t values where XX >= 32
       without signed overflow, which would be undefined behaviour. */
    STATIC_ASSERT(UINT_MAX <= UINT32_MAX);

    /* Conversions from unsigned to signed outside of the bounds of the signed type are
       implementation-defined. Verify that they function as reinterpreting the lower
       bits of the input in two's complement notation. Do this for conversions:
       - from uint(N)_t to int(N)_t with negative result
       - from uint(2N)_t to int(N)_t with negative result
       - from int(2N)_t to int(N)_t with negative result
       - from int(2N)_t to int(N)_t with positive result */

    /* To int8_t. */
    STATIC_ASSERT(((int8_t)(uint8_t)0xAB == (int8_t)-(int8_t)0x55));
    STATIC_ASSERT((int8_t)(uint16_t)0xABCD == (int8_t)-(int8_t)0x33);
    STATIC_ASSERT((int8_t)(int16_t)(uint16_t)0xCDEF == (int8_t)(uint8_t)0xEF);
    STATIC_ASSERT((int8_t)(int16_t)(uint16_t)0x9234 == (int8_t)(uint8_t)0x34);

    /* To int16_t. */
    STATIC_ASSERT((int16_t)(uint16_t)0xBCDE == (int16_t)-(int16_t)0x4322);
    STATIC_ASSERT((int16_t)(uint32_t)0xA1B2C3D4 == (int16_t)-(int16_t)0x3C2C);
    STATIC_ASSERT((int16_t)(int32_t)(uint32_t)0xC1D2E3F4 == (int16_t)(uint16_t)0xE3F4);
    STATIC_ASSERT((int16_t)(int32_t)(uint32_t)0x92345678 == (int16_t)(uint16_t)0x5678);

    /* To int32_t. */
    STATIC_ASSERT((int32_t)(uint32_t)0xB2C3D4E5 == (int32_t)-(int32_t)0x4D3C2B1B);
    STATIC_ASSERT((int32_t)(uint64_t)0xA123B456C789D012ULL == (int32_t)-(int32_t)0x38762FEE);
    STATIC_ASSERT((int32_t)(int64_t)(uint64_t)0xC1D2E3F4A5B6C7D8ULL == (int32_t)(uint32_t)0xA5B6C7D8);
    STATIC_ASSERT((int32_t)(int64_t)(uint64_t)0xABCDEF0123456789ULL == (int32_t)(uint32_t)0x23456789);

    /* To int64_t. */
    STATIC_ASSERT((int64_t)(uint64_t)0xB123C456D789E012ULL == (int64_t)-(int64_t)0x4EDC3BA928761FEEULL);
#if defined(SECP256K1_INT128_NATIVE)
    STATIC_ASSERT((int64_t)(((uint128_t)0xA1234567B8901234ULL << 64) + 0xC5678901D2345678ULL) == (int64_t)-(int64_t)0x3A9876FE2DCBA988ULL);
    STATIC_ASSERT(((int64_t)(int128_t)(((uint128_t)0xB1C2D3E4F5A6B7C8ULL << 64) + 0xD9E0F1A2B3C4D5E6ULL)) == (int64_t)(uint64_t)0xD9E0F1A2B3C4D5E6ULL);
    STATIC_ASSERT(((int64_t)(int128_t)(((uint128_t)0xABCDEF0123456789ULL << 64) + 0x0123456789ABCDEFULL)) == (int64_t)(uint64_t)0x0123456789ABCDEFULL);

    /* To int128_t. */
    STATIC_ASSERT((int128_t)(((uint128_t)0xB1234567C8901234ULL << 64) + 0xD5678901E2345678ULL) == (int128_t)(-(int128_t)0x8E1648B3F50E80DCULL * 0x8E1648B3F50E80DDULL + 0x5EA688D5482F9464ULL));
#endif

    /* Right shift on negative signed values is implementation defined. Verify that it
       acts as a right shift in two's complement with sign extension (i.e duplicating
       the top bit into newly added bits). */
    STATIC_ASSERT((((int8_t)0xE8) >> 2) == (int8_t)(uint8_t)0xFA);
    STATIC_ASSERT((((int16_t)0xE9AC) >> 4) == (int16_t)(uint16_t)0xFE9A);
    STATIC_ASSERT((((int32_t)0x937C918A) >> 9) == (int32_t)(uint32_t)0xFFC9BE48);
    STATIC_ASSERT((((int64_t)0xA8B72231DF9CF4B9ULL) >> 19) == (int64_t)(uint64_t)0xFFFFF516E4463BF3ULL);
#if defined(SECP256K1_INT128_NATIVE)
    STATIC_ASSERT((((int128_t)(((uint128_t)0xCD833A65684A0DBCULL << 64) + 0xB349312F71EA7637ULL)) >> 39) == (int128_t)(((uint128_t)0xFFFFFFFFFF9B0674ULL << 64) + 0xCAD0941B79669262ULL));
#endif

    /* This function is not supposed to be called. */
    VERIFY_CHECK(0);
}

#endif /* SECP256K1_ASSUMPTIONS_H */
