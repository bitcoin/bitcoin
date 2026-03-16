/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_TESTS_COMMON_H
#define SECP256K1_TESTS_COMMON_H

/***********************************************************************
 * Test Support Utilities
 *
 * This file provides general-purpose functions for tests and benchmark
 * programs. Unlike testutil.h, this file is not linked to the library,
 * allowing each program to choose whether to run against the production
 * API or access library internals directly.
 ***********************************************************************/

#include <stdint.h>

#if (defined(_MSC_VER) && _MSC_VER >= 1900)
#  include <time.h>
#else
#  include <sys/time.h>
#endif

static int64_t gettime_i64(void) {
#if (defined(_MSC_VER) && _MSC_VER >= 1900)
    /* C11 way to get wallclock time */
    struct timespec tv;
    if (!timespec_get(&tv, TIME_UTC)) {
        fputs("timespec_get failed!", stderr);
        exit(EXIT_FAILURE);
    }
    return (int64_t)tv.tv_nsec / 1000 + (int64_t)tv.tv_sec * 1000000LL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_usec + (int64_t)tv.tv_sec * 1000000LL;
#endif
}

#endif /* SECP256K1_TESTS_COMMON_H */
