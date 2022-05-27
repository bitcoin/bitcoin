// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/checks.h>

#include <key.h>
#include <node/ui_interface.h>
#include <random.h>
#include <util/time.h>
#include <util/translation.h>

#include <memory>

namespace kernel {

bool SanityChecks(const Context&) {
    if (!ECC_InitSanityCheck()) {
        return InitError(Untranslated("Elliptic curve cryptography sanity check failure. Aborting."));
    }

    if (!Random_SanityCheck()) {
        return InitError(Untranslated("OS cryptographic RNG sanity check failure. Aborting."));
    }

    if (!ChronoSanityCheck()) {
        return InitError(Untranslated("Clock epoch mismatch. Aborting."));
    }

    return true;
}

}
