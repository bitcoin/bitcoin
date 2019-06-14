/**
 * @file uint256_extensions.h
 *
 * This file provides helper to handle uint256 calculations.
 */

#ifndef OMNICORE_UINT256_EXTENSIONS_H
#define OMNICORE_UINT256_EXTENSIONS_H

#include "arith_uint256.h"

#include <assert.h>
#include <stdint.h>
#include <limits>

namespace mastercore {
namespace uint256_const {
//! The number 1 as uint256
static const arith_uint256 one(static_cast<uint64_t>(1));
//! The highest number in range of int64_t as uint256
static const arith_uint256 max_int64(static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
}

/**
 * Returns a mod n.
 */
inline arith_uint256 Modulo256(const arith_uint256& a, const arith_uint256& n)
{
    return (a - (n * (a / n)));
}

/**
 * Converts a positive primitive number to uint256.
 */
template<typename NumberT>
inline arith_uint256 ConvertTo256(const NumberT& number)
{
    assert(number >= 0);
    return arith_uint256(static_cast<uint64_t>(number));
}

/**
 * Converts an uint256 to int64_t.
 */
inline int64_t ConvertTo64(const arith_uint256& number)
{
    assert(number <= uint256_const::max_int64);
    return static_cast<int64_t>(number.GetLow64());
}

/**
 * Returns ceil(numerator / denominator).
 */
inline arith_uint256 DivideAndRoundUp(const arith_uint256& numerator, const arith_uint256& denominator)
{
    if (numerator == 0) {
        return arith_uint256(0);
    }
    return uint256_const::one + (numerator - uint256_const::one) / denominator;
}

} // namespace mastercore

#endif // OMNICORE_UINT256_EXTENSIONS_H
