// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/sanity.h>
#include <crypto/sha256.h>
#include <key.h>
#include <logging.h>
#include <node/ui_interface.h>
#include <pubkey.h>
#include <random.h>
#include <util/time.h>
#include <util/translation.h>

#include <memory>

static std::unique_ptr<ECCVerifyHandle> globalVerifyHandle;

namespace init {
void SetGlobals()
{
    std::string sha256_algo = SHA256AutoDetect();
    LogPrintf("Using the '%s' SHA256 implementation\n", sha256_algo);
    RandomInit();
    ECC_Start();
    globalVerifyHandle.reset(new ECCVerifyHandle());
}

void UnsetGlobals()
{
    globalVerifyHandle.reset();
    ECC_Stop();
}

bool SanityChecks()
{
    if (!ECC_InitSanityCheck()) {
        return InitError(Untranslated("Elliptic curve cryptography sanity check failure. Aborting."));
    }

    if (!glibcxx_sanity_test())
        return false;

    if (!Random_SanityCheck()) {
        return InitError(Untranslated("OS cryptographic RNG sanity check failure. Aborting."));
    }

    if (!ChronoSanityCheck()) {
        return InitError(Untranslated("Clock epoch mismatch. Aborting."));
    }

    return true;
}
} // namespace init
