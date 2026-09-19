// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_ECC_INIT_H
#define BITCOIN_COMMON_ECC_INIT_H

#include <memory>

class ECC_Context;
struct ECC_ContextDeleter {
    void operator()(ECC_Context*) const;
};

/** Check that required EC support is available at runtime. */
bool ECC_InitSanityCheck();

/** Initialize elliptic curve support. */
std::unique_ptr<ECC_Context, ECC_ContextDeleter> MakeContextECC();

#endif // BITCOIN_COMMON_ECC_INIT_H
