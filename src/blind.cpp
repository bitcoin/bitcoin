// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "blind.h"

#include <assert.h>
#include <secp256k1.h>

secp256k1_context *secp256k1_ctx_blind = NULL;

void ECC_Start_Blinding()
{
    assert(secp256k1_ctx_blind == NULL);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    assert(ctx != NULL);

    secp256k1_ctx_blind = ctx;
};

void ECC_Stop_Blinding()
{
    secp256k1_context *ctx = secp256k1_ctx_blind;
    secp256k1_ctx_blind = NULL;

    if (ctx)
    {
        secp256k1_context_destroy(ctx);
    };
};
