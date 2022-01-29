#pragma once

#include <mw/crypto/Pedersen.h>

class Blinds
{
public:
    Blinds() = default;
    Blinds(BlindingFactor blind)
        : m_positive{std::move(blind)}, m_negative{} { }

    static Blinds From(BlindingFactor blind) { return Blinds(std::move(blind)); }
    static Blinds From(const SecretKey& key) { return Blinds(BlindingFactor(key)); }

    Blinds& Add(BlindingFactor blind)
    {
        m_positive.push_back(std::move(blind));
        return *this;
    }

    Blinds& Add(const BigInt<32>& blind)
    {
        m_positive.push_back(BlindingFactor(blind));
        return *this;
    }

    Blinds& Add(const SecretKey& key)
    {
        m_positive.push_back(BlindingFactor(key));
        return *this;
    }

    Blinds& Add(const std::vector<BlindingFactor>& blinds)
    {
        m_positive.insert(m_positive.end(), blinds.cbegin(), blinds.cend());
        return *this;
    }

    Blinds& Add(const std::vector<SecretKey>& keys)
    {
        std::transform(
            keys.cbegin(), keys.cend(), std::back_inserter(m_positive),
            [](const SecretKey& key) { return BlindingFactor(key.GetBigInt()); });
        return *this;
    }

    Blinds& Sub(BlindingFactor blind)
    {
        m_negative.push_back(std::move(blind));
        return *this;
    }

    Blinds& Sub(const BigInt<32>& blind)
    {
        m_negative.push_back(BlindingFactor(blind));
        return *this;
    }

    Blinds& Sub(const SecretKey& key)
    {
        m_negative.push_back(BlindingFactor(key.GetBigInt()));
        return *this;
    }

    Blinds& Sub(const std::vector<BlindingFactor>& blinds)
    {
        m_negative.insert(m_negative.end(), blinds.cbegin(), blinds.cend());
        return *this;
    }

    Blinds& Sub(const std::vector<SecretKey>& keys)
    {
        std::transform(
            keys.cbegin(), keys.cend(), std::back_inserter(m_negative),
            [](const SecretKey& key) { return BlindingFactor(key.GetBigInt()); }
        );
        return *this;
    }

    BlindingFactor Total() const { return Pedersen::AddBlindingFactors(m_positive, m_negative); }
    SecretKey ToKey() const { return Pedersen::AddBlindingFactors(m_positive, m_negative).data(); }

private:
    std::vector<BlindingFactor> m_positive;
    std::vector<BlindingFactor> m_negative;
};