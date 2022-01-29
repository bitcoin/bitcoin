#include <mw/crypto/SecretKeys.h>
#include "Context.h"

static Locked<Context> SECKEY_CONTEXT(std::make_shared<Context>());

SecretKeys SecretKeys::From(const SecretKey& secret_key)
{
    const int verify_result = secp256k1_ec_seckey_verify(SECKEY_CONTEXT.Read()->Get(), secret_key.data());
    if (verify_result != 1) {
        ThrowCrypto("secp256k1_ec_seckey_verify failed");
    }

    return SecretKeys(secret_key);
}

SecretKeys& SecretKeys::Add(const SecretKey& secret_key)
{
    const int tweak_result = secp256k1_ec_privkey_tweak_add(
        SECKEY_CONTEXT.Read()->Get(),
        m_key.data(),
        secret_key.data()
    );
    if (tweak_result != 1) {
        ThrowCrypto("secp256k1_ec_privkey_tweak_add failed");
    }

    return *this;
}

SecretKeys& SecretKeys::Mul(const SecretKey& secret_key)
{
    const int tweak_result = secp256k1_ec_privkey_tweak_mul(
        SECKEY_CONTEXT.Read()->Get(),
        m_key.data(),
        secret_key.data()
    );
    if (tweak_result != 1) {
        ThrowCrypto("secp256k1_ec_privkey_tweak_mul failed");
    }

    return *this;
}