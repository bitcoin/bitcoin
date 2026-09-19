// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/ecc_init.h>

#include <ecc_context.h>
#include <key.h>
#include <pubkey.h>
#include <random.h>
#include <support/allocators/secure.h>

#include <memory>
#include <span>
#include <vector>

bool ECC_InitSanityCheck() {
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();
    return key.VerifyPubKey(pubkey);
}

static std::vector<unsigned char, secure_allocator<unsigned char>> RandSeed32()
{
    std::vector<unsigned char, secure_allocator<unsigned char>> rng_seed(32);
    GetRandBytes(rng_seed);
    return rng_seed;
}

void ECC_ContextDeleter::operator()(ECC_Context* ctx) const { delete ctx; }

std::unique_ptr<ECC_Context, ECC_ContextDeleter> MakeContextECC()
{
    return std::unique_ptr<ECC_Context, ECC_ContextDeleter>{new ECC_Context{RandSeed32()}};
}
