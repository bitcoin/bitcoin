/**********************************************************************
 * Copyright (c) 2020 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _MINISKETCH_FALSE_POSITIVES_H_
#define _MINISKETCH_FALSE_POSITIVES_H_

#include "util.h"

#include "int_utils.h"

#include <stdint.h>

namespace {

/** Compute floor(log2(x!)), exactly up to x=57; an underestimate up to x=2^32-1. */
uint64_t Log2Factorial(uint32_t x) {
    //! Values of floor(106*log2(1 + i/32)) for i=0..31
    static constexpr uint8_t T[32] = {
        0, 4, 9, 13, 18, 22, 26, 30, 34, 37, 41, 45, 48, 52, 55, 58, 62, 65, 68,
        71, 74, 77, 80, 82, 85, 88, 90, 93, 96, 98, 101, 103
    };
    int bits = CountBits(x, 32);
    // Compute an (under)estimate of floor(106*log2(x)).
    // This works by relying on floor(log2(x)) = countbits(x)-1, and adding
    // precision using the top 6 bits of x (the highest one of which is always
    // one).
    unsigned l2_106 = 106 * (bits - 1) + T[((x << (32 - bits)) >> 26) & 31];
    // Based on Stirling approximation for log2(x!):
    //   log2(x!) = log(x!) / log(2)
    //            = ((x + 1/2) * log(x) - x + log(2*pi)/2 + ...) / log(2)
    //            = (x + 1/2) * log2(x) - x/log(2) + log2(2*pi)/2 + ...
    //            = 1/2*(2*x+1)*log2(x) - (1/log(2))*x + log2(2*pi)/2 + ...
    //            = 1/212*(2*x+1)*(106*log2(x)) + (-1/log(2))*x + log2(2*pi)/2 + ...
    // where 418079/88632748 is exactly 1/212
    //       -127870026/88632748 is slightly less than -1/log(2)
    //       117504694/88632748 is less than log2(2*pi)/2
    // A correction term is only needed for x < 3.
    //
    // See doc/log2_factorial.sage for how these constants were obtained.
    return (418079 * (2 * uint64_t{x} + 1) * l2_106 - 127870026 * uint64_t{x} + 117504694 + 88632748 * (x < 3)) / 88632748;
}

/** Compute floor(log2(2^(bits * capacity) / sum((2^bits - 1) choose k, k=0..capacity))), for bits>1
 *
 * See doc/gen_basefpbits.sage for how the tables were obtained. */
uint64_t BaseFPBits(uint32_t bits, uint32_t capacity) {
    // Correction table for low bits/capacities
    static constexpr uint8_t ADD5[] = {1, 1, 1, 1, 2, 2, 2, 3, 4, 4, 5, 5, 6, 7, 8, 8, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12};
    static constexpr uint8_t ADD6[] = {1, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 4, 5, 6, 6, 6, 7, 8, 8, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 20, 21, 21, 22, 22, 23, 23, 23, 24, 24, 24, 24};
    static constexpr uint8_t ADD7[] = {1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 7, 8, 9, 9, 9, 10, 11, 11, 12, 12, 13, 13, 15, 15, 15, 16, 17, 17, 18, 19, 20, 20};
    static constexpr uint8_t ADD8[] = {1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 3, 4, 4, 5, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 8, 9, 9};
    static constexpr uint8_t ADD9[] = {1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 2, 1, 1, 1, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 3, 2, 3, 3, 3, 3, 4, 3, 3, 4, 4, 4, 4};

    if (capacity == 0) return 0;
    uint64_t ret = 0;
    if (bits < 32 && capacity >= (1U << bits)) {
        ret = uint64_t{bits} * (capacity - (1U << bits) + 1);
        capacity = (1U << bits) - 1;
    }
    ret += Log2Factorial(capacity);
    switch (bits) {
        case 2: return ret + (capacity <= 2 ? 0 : 1);
        case 3: return ret + (capacity <= 2 ? 0 : (0x2a5 >> 2 * (capacity - 3)) & 3);
        case 4: return ret + (capacity <= 3 ? 0 : (0xb6d91a449 >> 3 * (capacity - 4)) & 7);
        case 5: return ret + (capacity <= 4 ? 0 : ADD5[capacity - 5]);
        case 6: return ret + (capacity <= 4 ? 0 : capacity > 54 ? 25 : ADD6[capacity - 5]);
        case 7: return ret + (capacity <= 4 ? 0 : capacity > 57 ? 21 : ADD7[capacity - 5]);
        case 8: return ret + (capacity <= 9 ? 0 : capacity > 56 ? 10 : ADD8[capacity - 10]);
        case 9: return ret + (capacity <= 11 ? 0 : capacity > 54 ? 5 : ADD9[capacity - 12]);
        case 10: return ret + (capacity <= 21 ? 0 : capacity > 50 ? 2 : (0x1a6665545555041 >> 2 * (capacity - 22)) & 3);
        case 11: return ret + (capacity <= 21 ? 0 : capacity > 45 ? 1 : (0x5b3dc1 >> (capacity - 22)) & 1);
        case 12: return ret + (capacity <= 21 ? 0 : capacity > 57 ? 0 : (0xe65522041 >> (capacity - 22)) & 1);
        case 13: return ret + (capacity <= 27 ? 0 : capacity > 55 ? 0 : (0x8904081 >> (capacity - 28)) & 1);
        case 14: return ret + (capacity <= 47 ? 0 : capacity > 48 ? 0 : 1);
        default: return ret;
    }
}

size_t ComputeCapacity(uint32_t bits, size_t max_elements, uint32_t fpbits) {
    if (bits == 0) return 0;
    uint64_t base_fpbits = BaseFPBits(bits, max_elements);
    // The fpbits provided by the base max_elements==capacity case are sufficient.
    if (base_fpbits >= fpbits) return max_elements;
    // Otherwise, increment capacity by ceil(fpbits / bits) beyond that.
    return max_elements + (fpbits - base_fpbits + bits - 1) / bits;
}

size_t ComputeMaxElements(uint32_t bits, size_t capacity, uint32_t fpbits) {
    if (bits == 0) return 0;
    // Start with max_elements=capacity, and decrease max_elements until the corresponding capacity is capacity.
    size_t max_elements = capacity;
    while (true) {
        size_t capacity_for_max_elements = ComputeCapacity(bits, max_elements, fpbits);
        CHECK_SAFE(capacity_for_max_elements >= capacity);
        if (capacity_for_max_elements <= capacity) return max_elements;
        size_t adjust = capacity_for_max_elements - capacity;
        // Decrementing max_elements by N will at most decrement the corresponding capacity by N.
        // As the observed capacity is adjust too high, we can safely decrease max_elements by adjust.
        // If that brings us into negative max_elements territory, no solution exists and we return 0.
        if (max_elements < adjust) return 0;
        max_elements -= adjust;
    }
}

}  // namespace

#endif
