// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util/check_globals.h>

#include <test/util/random.h>
#include <util/time.h>

#include <iostream>
#include <memory>
#include <optional>
#include <string>

struct CheckGlobalsImpl {
    CheckGlobalsImpl()
    {
        g_used_g_prng = false;
        g_seeded_g_prng_zero = false;
        g_used_system_time = false;
        SetMockTime(0s);
    }
    ~CheckGlobalsImpl()
    {
        if (g_used_g_prng && !g_seeded_g_prng_zero) {
            std::cerr << "\n\n"
                         "The current fuzz target used the global random state.\n\n"

                         "This is acceptable, but requires the fuzz target to call \n"
                         "SeedRandomStateForTest(SeedRand::ZEROS) in the first line \n"
                         "of the FUZZ_TARGET function.\n\n"

                         "An alternative solution would be to avoid any use of globals.\n\n"

                         "Without a solution, fuzz instability and non-determinism can lead \n"
                         "to non-reproducible bugs or inefficient fuzzing.\n\n"
                      << std::endl;
            std::abort(); // Abort, because AFL may try to recover from a std::exit
        }

        if (g_used_system_time) {
            std::cerr << "\n\n"
                         "The current fuzz target accessed system time.\n\n"

                         "This is acceptable, but requires the fuzz target to call \n"
                         "SetMockTime() at the beginning of processing the fuzz input.\n\n"

                         "Without setting mock time, time-dependent behavior can lead \n"
                         "to non-reproducible bugs or inefficient fuzzing.\n\n"
                      << std::endl;
            std::abort();
        }
    }
};

CheckGlobals::CheckGlobals() : m_impl(std::make_unique<CheckGlobalsImpl>()) {}
CheckGlobals::~CheckGlobals() = default;
