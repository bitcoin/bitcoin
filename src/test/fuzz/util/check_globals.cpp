// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util/check_globals.h>

#include <test/util/random.h>

#include <iostream>
#include <memory>
#include <optional>
#include <string>

struct CheckGlobalsImpl {
    CheckGlobalsImpl()
    {
        g_used_g_prng = false;
        g_seeded_g_prng_zero = false;
    }
    ~CheckGlobalsImpl()
    {
        if (g_used_g_prng && !g_seeded_g_prng_zero) {
            std::cerr << "\n\n"
                         "The current fuzz target used the global random state.\n\n"

                         "This is acceptable, but requires the fuzz target to call \n"
                         "SeedRandomStateForTest(SeedRand::ZEROS) at the beginning \n"
                         "of processing the fuzz input.\n\n"

                         "An alternative solution would be to avoid any use of globals.\n\n"

                         "Without a solution, fuzz stability and determinism can lead \n"
                         "to non-reproducible bugs or inefficient fuzzing.\n\n"
                      << std::endl;
            std::abort(); // Abort, because AFL may try to recover from a std::exit
        }
    }
};

CheckGlobals::CheckGlobals() : m_impl(std::make_unique<CheckGlobalsImpl>()) {}
CheckGlobals::~CheckGlobals() = default;
