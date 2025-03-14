// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/coverage.h>

#if defined(__clang__)
extern "C" void __llvm_profile_reset_counters(void) __attribute__((weak));
extern "C" void __gcov_reset(void) __attribute__((weak));

// Fallback implementations
extern "C" __attribute__((weak)) void __llvm_profile_reset_counters(void) {}
extern "C" __attribute__((weak)) void __gcov_reset(void) {}

void ResetCoverageCounters() {
    // These will call the real ones if available, or our dummies if not
    __llvm_profile_reset_counters();
    __gcov_reset();
}
#else
void ResetCoverageCounters() {}
#endif
