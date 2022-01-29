#pragma once

#include <mw/crypto/PublicKeys.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/crypto/Commitment.h>
#include <mw/models/crypto/PublicKey.h>
#include <mw/models/crypto/SecretKey.h>

// FUTURE: Make this more efficient
class Keys
{
public:
    static Keys From(const PublicKey& public_key)
    {
        return Keys(public_key);
    }

    static Keys From(const SecretKey& secret_key)
    {
        return Keys(PublicKey::From(secret_key));
    }

    static Keys Random()
    {
        return Keys(PublicKey::From(SecretKey::Random()));
    }

    const PublicKey& PubKey() const noexcept { return m_pubkey; }

    Keys& Add(const PublicKey& public_key)
    {
        m_pubkey = PublicKeys::Add({m_pubkey, public_key});
        return *this;
    }

    Keys& Add(const SecretKey& secret_key)
    {
        m_pubkey = PublicKeys::Add({m_pubkey, PublicKey::From(secret_key)});
        return *this;
    }

    Keys& Add(const std::vector<PublicKey>& public_keys)
    {
        std::vector<PublicKey> pubkeys = public_keys;
        pubkeys.push_back(m_pubkey);
        m_pubkey = PublicKeys::Add(pubkeys);
        return *this;
    }

    Keys& Mul(const SecretKey& secret_key)
    {
        m_pubkey = PublicKeys::MultiplyKey(m_pubkey, secret_key);
        return *this;
    }

private:
    Keys(const PublicKey& pubkey)
        : m_pubkey(pubkey) { }

    PublicKey m_pubkey;
};