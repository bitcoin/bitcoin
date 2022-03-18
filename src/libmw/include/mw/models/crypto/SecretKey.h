#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Traits.h>
#include <mw/models/crypto/BigInteger.h>
#include <random.h>

template<size_t NUM_BYTES>
class secret_key_t : public Traits::ISerializable
{
public:
    //
    // Constructor
    //
    secret_key_t() = default;
    secret_key_t(BigInt<NUM_BYTES>&& value) : m_value(std::move(value)) { }
    secret_key_t(std::vector<uint8_t>&& bytes) : m_value(BigInt<NUM_BYTES>(std::move(bytes))) { }
    secret_key_t(const std::array<uint8_t, NUM_BYTES>& bytes) : m_value(BigInt<NUM_BYTES>(bytes)) {}
    secret_key_t(std::array<uint8_t, NUM_BYTES>&& bytes) : m_value(BigInt<NUM_BYTES>(std::move(bytes))) { }
    secret_key_t(const uint8_t* bytes) : m_value(BigInt<NUM_BYTES>(bytes)) { }

    static secret_key_t Null() { return secret_key_t<NUM_BYTES>(); }

    static secret_key_t Random()
    {
        secret_key_t<NUM_BYTES> key;

        size_t index = 0;
        while (index < NUM_BYTES) {
            size_t num_bytes = std::min(NUM_BYTES - index, (size_t)32);
            GetStrongRandBytes(key.data() + index, num_bytes);
            index += num_bytes;
        }
        return key;
    }

    //
    // Destructor
    //
    virtual ~secret_key_t() = default;

    //
    // Operators
    //
    bool operator==(const secret_key_t<NUM_BYTES>& rhs) const noexcept { return m_value == rhs.m_value; }

    //
    // Getters
    //
    const BigInt<NUM_BYTES>& GetBigInt() const { return m_value; }
    std::string ToHex() const noexcept { return m_value.ToHex(); }
    bool IsNull() const noexcept { return m_value.IsZero(); }
    const std::vector<uint8_t>& vec() const { return m_value.vec(); }
    std::array<uint8_t, 32> array() const noexcept { return m_value.ToArray(); }
    uint8_t* data() { return m_value.data(); }
    const uint8_t* data() const { return m_value.data(); }
    uint8_t& operator[] (const size_t x) { return m_value[x]; }
    const uint8_t& operator[] (const size_t x) const { return m_value[x]; }
    size_t size() const { return m_value.size(); }

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(secret_key_t<NUM_BYTES>, obj)
    {
        READWRITE(obj.m_value);
    }

private:
    BigInt<NUM_BYTES> m_value;
};

using SecretKey = secret_key_t<32>;
using SecretKey64 = secret_key_t<64>;