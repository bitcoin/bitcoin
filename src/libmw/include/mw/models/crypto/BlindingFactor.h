#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Traits.h>
#include <mw/models/crypto/BigInteger.h>
#include <mw/models/crypto/SecretKey.h>

class BlindingFactor : public Traits::ISerializable
{
public:
    //
    // Constructors
    //
    BlindingFactor() = default;
    BlindingFactor(BigInt<32>&& value) : m_value(std::move(value)) { }
    BlindingFactor(const BigInt<32>& value) : m_value(value) { }
    BlindingFactor(const std::array<uint8_t, 32>& value) : m_value(value) { }
    BlindingFactor(const SecretKey& key) : m_value(key.GetBigInt()) { }
    BlindingFactor(const uint8_t* data) : m_value(data) { }
    BlindingFactor(const BlindingFactor& other) = default;
    BlindingFactor(BlindingFactor&& other) noexcept = default;

    static BlindingFactor Random()
    {
        return BlindingFactor(SecretKey::Random());
    }

    //
    // Operators
    //
    BlindingFactor& operator=(const BlindingFactor& other) = default;
    BlindingFactor& operator=(BlindingFactor&& other) noexcept = default;
    bool operator<(const BlindingFactor& rhs) const noexcept { return m_value < rhs.GetBigInt(); }
    bool operator!=(const BlindingFactor& rhs) const noexcept { return m_value != rhs.GetBigInt(); }
    bool operator==(const BlindingFactor& rhs) const noexcept { return m_value == rhs.GetBigInt(); }

    //
    // Getters
    //
    const BigInt<32>& GetBigInt() const noexcept { return m_value; }
    const std::vector<uint8_t>& vec() const noexcept { return m_value.vec(); }
    std::array<uint8_t, 32> array() const noexcept { return m_value.ToArray(); }
    const uint8_t* data() const noexcept { return m_value.data(); }
    uint8_t* data() noexcept { return m_value.data(); }
    size_t size() const noexcept { return m_value.size(); }
    bool IsZero() const noexcept { return m_value.IsZero(); }

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(BlindingFactor, obj)
    {
        READWRITE(obj.m_value);
    }

    std::string ToHex() const { return m_value.ToHex(); }
    static BlindingFactor FromHex(const std::string& hex) { return BlindingFactor(BigInt<32>::FromHex(hex)); }

private:
    // The 32 byte blinding factor.
    BigInt<32> m_value;
};