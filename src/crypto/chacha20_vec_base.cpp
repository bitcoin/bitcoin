// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define CHACHA20_NAMESPACE chacha20_vec_base

// This file should define which states should be en/disabled for all
// supported architectures. For some, like x86-64 and armv8, simd features
// (sse2 and neon respectively) are safe to use without runtime detection.

#if defined(__x86_64__) || defined(__amd64__)
#  define CHACHA20_VEC_DISABLE_STATES_16
#  define CHACHA20_VEC_DISABLE_STATES_8
#  define CHACHA20_VEC_DISABLE_STATES_6
#elif defined(__ARM_NEON)
#  define CHACHA20_VEC_DISABLE_STATES_2
#else
// Be conservative and require platforms to opt-in
#  define CHACHA20_VEC_DISABLE_STATES_16
#  define CHACHA20_VEC_DISABLE_STATES_8
#  define CHACHA20_VEC_DISABLE_STATES_6
#  define CHACHA20_VEC_DISABLE_STATES_4
#  define CHACHA20_VEC_DISABLE_STATES_2
#endif

#include <crypto/chacha20_vec.ipp>
