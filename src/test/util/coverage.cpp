// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/coverage.h>

#if defined(__clang__) && defined(__linux__)
extern "C" void __llvm_profile_reset_counters(void) __attribute__((weak));
extern "C" void __gcov_reset(void) __attribute__((weak));

void ResetCoverageCounters()
{
    if (__llvm_profile_reset_counters) {
        __llvm_profile_reset_counters();
    }

    if (__gcov_reset) {
        __gcov_reset();
    }
}
#else
void ResetCoverageCounters() {}
#endif
