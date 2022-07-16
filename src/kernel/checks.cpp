// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/checks.h>

#include <key.h>
#include <random.h>
#include <util/time.h>

namespace kernel {

std::optional<SanityCheckError> SanityChecks(const Context&)
{
    if (!ECC_InitSanityCheck()) {
        return SanityCheckError::ERROR_ECC;
    }

    if (!Random_SanityCheck()) {
        return SanityCheckError::ERROR_RANDOM;
    }

    if (!ChronoSanityCheck()) {
        return SanityCheckError::ERROR_CHRONO;
    }

    return std::nullopt;
}

}
