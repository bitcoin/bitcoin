// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Compile-time verification of assumptions we make.

#ifndef BITCOIN_COMPAT_ASSUMPTIONS_H
#define BITCOIN_COMPAT_ASSUMPTIONS_H

#include <limits>

// Assumption: We assume that the macro NDEBUG is not defined.
// Example(s): We use assert(...) extensively with the assumption of it never
//             being a noop at runtime.
#if defined(NDEBUG)
# error "Bitcoin cannot be compiled without assertions."
#endif

// Assumption: We assume a C++17 (ISO/IEC 14882:2017) compiler (minimum requirement).
// Example(s): We assume the presence of C++17 features everywhere :-)
// ISO Standard C++17 [cpp.predefined]p1:
// "The name __cplusplus is defined to the value 201703L when compiling a C++
//  translation unit."
static_assert(__cplusplus >= 201703L, "C++17 standard assumed");

// Assumption: We assume the floating-point types to fulfill the requirements of
//             IEC 559 (IEEE 754) standard.
// Example(s): Floating-point division by zero in ConnectBlock, CreateTransaction
//             and EstimateMedianVal.
static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 float assumed");
static_assert(std::numeric_limits<double>::is_iec559, "IEEE 754 double assumed");

// Assumption: We assume eight bits per byte (obviously, but remember: don't
//             trust -- verify!).
// Example(s): Everywhere :-)
static_assert(std::numeric_limits<unsigned char>::digits == 8, "8-bit byte assumed");

// Assumption: We assume floating-point widths.
// Example(s): Type punning in serialization code (ser_{float,double}_to_uint{32,64}).
static_assert(sizeof(float) == 4, "32-bit float assumed");
static_assert(sizeof(double) == 8, "64-bit double assumed");

// Assumption: We assume integer widths.
// Example(s): GetSizeOfCompactSize and WriteCompactSize in the serialization
//             code.
static_assert(sizeof(short) == 2, "16-bit short assumed");
static_assert(sizeof(int) == 4, "32-bit int assumed");
static_assert(sizeof(unsigned) == 4, "32-bit unsigned assumed");

// Assumption: We assume size_t to be 32-bit or 64-bit.
// Example(s): size_t assumed to be at least 32-bit in ecdsa_signature_parse_der_lax(...).
//             size_t assumed to be 32-bit or 64-bit in MallocUsage(...).
static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "size_t assumed to be 32-bit or 64-bit");
static_assert(sizeof(size_t) == sizeof(void*), "Sizes of size_t and void* assumed to be equal");

// Some important things we are NOT assuming (non-exhaustive list):
// * We are NOT assuming a specific value for std::endian::native.
// * We are NOT assuming a specific value for std::locale("").name().
// * We are NOT assuming a specific value for std::numeric_limits<char>::is_signed.

#endif // BITCOIN_COMPAT_ASSUMPTIONS_H
