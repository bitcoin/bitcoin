#pragma once

#include "secp256k1-zkp.h"

#include <mw/common/Lock.h>
#include <mw/models/crypto/SecretKey.h>
#include <mw/exceptions/CryptoException.h>

class Context
{
public:
    Context()
    {
        m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        m_pGenerators = secp256k1_bulletproof_generators_create(m_pContext, &secp256k1_generator_const_g, 256);
    }

    ~Context()
    {
        secp256k1_bulletproof_generators_destroy(m_pContext, m_pGenerators);
        secp256k1_context_destroy(m_pContext);
    }

    secp256k1_context* Randomized()
    {
        const int randomizeResult = secp256k1_context_randomize(m_pContext, SecretKey::Random().data());
        if (randomizeResult != 1) {
            ThrowCrypto("Context randomization failed.");
        }

        return m_pContext;
    }

    secp256k1_context* Get() noexcept { return m_pContext; }
    const secp256k1_context* Get() const noexcept { return m_pContext; }

    const secp256k1_bulletproof_generators* GetGenerators() const noexcept { return m_pGenerators; }

private:
    secp256k1_context* m_pContext;
    secp256k1_bulletproof_generators* m_pGenerators;
};