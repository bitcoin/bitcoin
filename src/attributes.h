// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ATTRIBUTES_H
#define BITCOIN_ATTRIBUTES_H

// Use ALLOW_WRAPAROUND to annotate functions that are known to cause unsigned integer
// wraparounds where the modular arithmetic is intentional (e.g. hash functions).
#if defined(__clang__)
#  define ALLOW_WRAPAROUND __attribute__((no_sanitize("unsigned-integer-overflow")))
#else
#  define ALLOW_WRAPAROUND
#endif

// Use WARNING_UNINTENTIONAL_WRAPAROUND to annotate existing functions that are known to cause
// unsigned integer wraparounds where the modular arithmetic is unintentional.
//
// Do not add new code with functions annotated WARNING_UNINTENTIONAL_WRAPAROUND. Newly introduced
// code should not trigger unintentional integer wraparounds.
#if defined(__clang__)
#  define WARNING_UNINTENTIONAL_WRAPAROUND __attribute__((no_sanitize("unsigned-integer-overflow")))
#else
#  define WARNING_UNINTENTIONAL_WRAPAROUND
#endif

#endif // BITCOIN_ATTRIBUTES_H
