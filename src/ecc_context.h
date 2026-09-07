// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ECC_CONTEXT_H
#define BITCOIN_ECC_CONTEXT_H

#include <span>

struct secp256k1_context_struct;
typedef struct secp256k1_context_struct secp256k1_context;

/** Access the secp256k1 context used for signing and MuSig2 nonce generation. */
secp256k1_context* GetSecp256k1SignContext();

/** Access the secp256k1 context used for verification: the ECC_Context's, or
 *  secp256k1_context_static without one. */
const secp256k1_context* GetSecp256k1VerifyContext();

/**
 * RAII class initializing and deinitializing global state for elliptic curve support.
 * Only one instance may be initialized at a time.
 */
class ECC_Context
{
public:
    explicit ECC_Context(std::span<const unsigned char> rng_seed32);
    ECC_Context(const ECC_Context&) = delete;
    ECC_Context& operator=(const ECC_Context&) = delete;

    ~ECC_Context();

    secp256k1_context* SignContext() const { return m_sign_ctx; }
    const secp256k1_context* VerifyContext() const { return m_verify_ctx; }

private:
    secp256k1_context* m_sign_ctx{nullptr};
    secp256k1_context* m_verify_ctx{nullptr};
};

#endif // BITCOIN_ECC_CONTEXT_H
