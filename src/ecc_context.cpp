// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ecc_context.h>

#include <crypto/sha256.h>
#include <secp256k1.h>

#include <cassert>
#include <span>

static const ECC_Context* g_ecc_context = nullptr;

secp256k1_context* GetSecp256k1SignContext()
{
    return g_ecc_context ? g_ecc_context->SignContext() : nullptr;
}

const secp256k1_context* GetSecp256k1VerifyContext()
{
    return g_ecc_context ? g_ecc_context->VerifyContext() : secp256k1_context_static;
}

/** Create an elliptic curve context. Provide rng seed for blinding factor if needed */
static secp256k1_context* ECC_Start(std::span<const unsigned char> rng_seed32) {
    assert(rng_seed32.empty() || rng_seed32.size() == 32);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    assert(ctx != nullptr);

    SHA256AutoDetect();
    secp256k1_context_set_sha256_compression(ctx, SHA256Transform);

    if (!rng_seed32.empty()){
        // Pass in a random blinding seed to the secp256k1 context.
        bool ret = secp256k1_context_randomize(ctx, rng_seed32.data());
        assert(ret);
    }

    return ctx;
}

ECC_Context::ECC_Context(std::span<const unsigned char> rng_seed32)
{
    assert(g_ecc_context == nullptr);
    m_sign_ctx = ECC_Start(rng_seed32);
    m_verify_ctx = ECC_Start(/*rng_seed32=*/{});
    g_ecc_context = this;
}

ECC_Context::~ECC_Context()
{
    assert(g_ecc_context == this);
    g_ecc_context = nullptr;
    secp256k1_context_destroy(m_verify_ctx);
    secp256k1_context_destroy(m_sign_ctx);
}
