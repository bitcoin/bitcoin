// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/** Functions for measurement of CPU cycles */
#ifndef BITCOIN_BENCH_PERF_H
#define BITCOIN_BENCH_PERF_H

#include <stdint.h>

#if defined(__i386__)

static inline uint64_t perf_cpucycles(void)
{
    uint64_t x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

static inline uint64_t perf_cpucycles(void)
{
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)lo)|(((uint64_t)hi)<<32);
}
#else

uint64_t perf_cpucycles(void);

#endif

void perf_init(void);
void perf_fini(void);

#endif // BITCOIN_BENCH_PERF_H
