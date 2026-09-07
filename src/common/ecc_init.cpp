// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/ecc_init.h>

#include <key.h>
#include <pubkey.h>

#include <memory>

bool ECC_InitSanityCheck() {
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();
    return key.VerifyPubKey(pubkey);
}

void ECC_ContextDeleter::operator()(ECC_Context* ctx) const { delete ctx; }

std::unique_ptr<ECC_Context, ECC_ContextDeleter> MakeContextECC()
{
    return std::unique_ptr<ECC_Context, ECC_ContextDeleter>{new ECC_Context{}};
}
