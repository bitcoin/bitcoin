// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/sha256.h>
#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>

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
} // namespace init
