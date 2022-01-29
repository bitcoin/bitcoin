#pragma once

#include <mw/models/crypto/SecretKey.h>

class SecretKeys
{
public:
    static SecretKeys From(const SecretKey& secret_key);

    static SecretKeys Random()
    {
        return SecretKeys::From(SecretKey::Random());
    }

    const SecretKey& Total() const noexcept { return m_key; }

    SecretKeys& Add(const SecretKey& secret_key);
    SecretKeys& Mul(const SecretKey& secret_key);

private:
    SecretKeys(const SecretKey& key)
        : m_key(key) {}

    SecretKey m_key;
};