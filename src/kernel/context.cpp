// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/context.h>

#include <crypto/sha256.h>
#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>

#include <string>
#include <bls/bls.h>

namespace kernel {

Context::Context()
{
    std::string sha256_algo = SHA256AutoDetect();
    LogPrintf("Using the '%s' SHA256 implementation\n", sha256_algo);
    RandomInit();
    ECC_Start();
    // SYSCOIN
    BLSInit();
}

Context::~Context()
{
    ECC_Stop();
}

} // namespace kernel
