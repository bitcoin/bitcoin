// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/checks.h>

#include <key.h>
#include <random.h>
#include <util/time.h>
#include <util/translation.h>

#include <memory>

namespace kernel {

std::optional<bilingual_str> SanityChecks(const Context&)
{
    if (!ECC_InitSanityCheck()) {
        return Untranslated("Elliptic curve cryptography sanity check failure. Aborting.");
    }

    if (!Random_SanityCheck()) {
        return Untranslated("OS cryptographic RNG sanity check failure. Aborting.");
    }

    if (!ChronoSanityCheck()) {
        return Untranslated("Clock epoch mismatch. Aborting.");
    }

    return std::nullopt;
}

}
