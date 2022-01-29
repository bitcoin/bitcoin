#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Traits.h>
#include <util/strencodings.h>

#include <cassert>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <array>

#ifdef _MSC_VER
#pragma warning(disable: 4505)
#endif

template<size_t NUM_BYTES, class ALLOC = std::allocator<uint8_t>>
class BigInt :
    public Traits::IPrintable,
    public Traits::ISerializable
{
public:
    //
    // Constructors
    //
    BigInt() noexcept : m_bytes(NUM_BYTES) { }
    BigInt(const std::vector<uint8_t, ALLOC>& bytes) : m_bytes(bytes) { assert(m_bytes.size() == NUM_BYTES); }
    BigInt(std::vector<uint8_t, ALLOC>&& bytes) : m_bytes(std::move(bytes)) { assert(m_bytes.size() == NUM_BYTES); }
    BigInt(const std::array<uint8_t, NUM_BYTES>& bytes) : m_bytes(bytes.cbegin(), bytes.cend()) { }
    explicit BigInt(const uint8_t* arr) : m_bytes(arr, arr + NUM_BYTES) { }
    BigInt(const BigInt& bigInteger) = default;
    BigInt(BigInt&& bigInteger) noexcept = default;

    //
    // Destructor
    //
    virtual ~BigInt()
    {
        for (size_t i = 0; i < m_bytes.size(); i++)
        {
            m_bytes[i] = 0;
        }
    }

    static size_t size() noexcept { return NUM_BYTES; }
    const std::vector<uint8_t, ALLOC>& vec() const { return m_bytes; }
    uint8_t* data() { return m_bytes.data(); }
    const uint8_t* data() const { return m_bytes.data(); }
    bool IsZero() const noexcept
    {
        assert(m_bytes.size() == NUM_BYTES);
        for (uint8_t byte : m_bytes)
        {
            if (byte != 0) {
                return false;
            }
        }

        return true;
    }

    static BigInt<NUM_BYTES, ALLOC> ValueOf(const uint8_t value)
    {
        std::vector<uint8_t, ALLOC> bytes(NUM_BYTES);
        bytes[NUM_BYTES - 1] = value;
        return BigInt<NUM_BYTES, ALLOC>(std::move(bytes));
    }

    static BigInt<NUM_BYTES, ALLOC> FromHex(const std::string& hex)
    {
        assert(hex.length() == NUM_BYTES * 2);
        std::vector<uint8_t> bytes = ParseHex(hex);
        assert(bytes.size() == NUM_BYTES);
        return BigInt<NUM_BYTES, ALLOC>(std::move(bytes));
    }

    static BigInt<NUM_BYTES, ALLOC> Max()
    {
        std::vector<uint8_t, ALLOC> bytes(NUM_BYTES);
        for (size_t i = 0; i < NUM_BYTES; i++)
        {
            bytes[i] = 0xFF;
        }

        return BigInt<NUM_BYTES, ALLOC>(std::move(bytes));
    }

    std::array<uint8_t, NUM_BYTES> ToArray() const noexcept
    {
        std::array<uint8_t, NUM_BYTES> arr;
        std::copy_n(m_bytes.begin(), NUM_BYTES, arr.begin());
        return arr;
    }

    std::string ToHex() const noexcept { return HexStr(m_bytes); }
    std::string Format() const noexcept final { return ToHex(); }

    //
    // Operators
    //
    BigInt& operator=(const BigInt& other) = default;
    BigInt& operator=(BigInt&& other) noexcept = default;

    BigInt operator^(const BigInt& rhs) const
    {
        BigInt<NUM_BYTES, ALLOC> result = *this;
        for (size_t i = 0; i < NUM_BYTES; i++)
        {
            result[i] ^= rhs[i];
        }

        return result;
    }

    uint8_t& operator[] (const size_t x) { return m_bytes[x]; }
    const uint8_t& operator[] (const size_t x) const { return m_bytes[x]; }

    bool operator<(const BigInt& rhs) const noexcept
    {
        if (this == &rhs)
        {
            return false;
        }

        assert(m_bytes.size() == NUM_BYTES && rhs.m_bytes.size() == NUM_BYTES);
        for (size_t i = 0; i < NUM_BYTES; i++)
        {
            if (m_bytes[i] != rhs.m_bytes[i])
            {
                return m_bytes[i] < rhs.m_bytes[i];
            }
        }

        return false;
    }

    bool operator>(const BigInt& rhs) const
    {
        return rhs < *this;
    }

    bool operator==(const BigInt& rhs) const
    {
        return this == &rhs || this->m_bytes == rhs.m_bytes;
    }

    bool operator!=(const BigInt& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<=(const BigInt& rhs) const
    {
        return *this < rhs || *this == rhs;
    }

    bool operator>=(const BigInt& rhs) const
    {
        return *this > rhs || *this == rhs;
    }

    BigInt operator^=(const BigInt& rhs)
    {
        *this = *this ^ rhs;

        return *this;
    }

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZED(BigInt);

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s.write((const char*)m_bytes.data(), NUM_BYTES);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s.read((char*)m_bytes.data(), NUM_BYTES);
    }

private:
    std::vector<uint8_t, ALLOC> m_bytes;
};