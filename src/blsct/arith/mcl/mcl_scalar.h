// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_BLSCT_ARITH_MCL_MCL_SCALAR_H
#define NAVCOIN_BLSCT_ARITH_MCL_MCL_SCALAR_H

#include <array>
#include <functional>
#include <stddef.h>
#include <string>
#include <vector>

#define BLS_ETH 1
#include <bls/bls384_256.h>
#include <hash.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <version.h>

using namespace std::literals::string_literals;

class MclScalar
{
public:
    using Underlying = mclBnFr;

    MclScalar();
    MclScalar(const int64_t& n);
    MclScalar(const std::vector<uint8_t>& v);
    template <size_t L>
    MclScalar(const std::array<uint8_t, L>& a);
    MclScalar(const Underlying& other_underlying);
    MclScalar(const uint256& n);
    MclScalar(const std::string& s, int radix);
    MclScalar(const std::vector<uint8_t>& msg, uint8_t index);

    MclScalar ApplyBitwiseOp(const MclScalar& a, const MclScalar& b,
                             std::function<uint8_t(uint8_t, uint8_t)> op) const;

    void operator=(const int64_t& n); // using int64_t instead of uint64_t since underlying mcl lib takes int64_t

    MclScalar operator+(const MclScalar& b) const;
    MclScalar operator-(const MclScalar& b) const;
    MclScalar operator*(const MclScalar& b) const;
    MclScalar operator/(const MclScalar& b) const;
    MclScalar operator|(const MclScalar& b) const;
    MclScalar operator^(const MclScalar& b) const;
    MclScalar operator&(const MclScalar& b) const;
    MclScalar operator~() const;
    MclScalar operator<<(const uint32_t& shift) const;
    MclScalar operator>>(const uint32_t& shift) const;

    bool operator==(const MclScalar& b) const;
    bool operator==(const int32_t& b) const;
    bool operator!=(const MclScalar& b) const;
    bool operator!=(const int32_t& b) const;

    bool operator<(const MclScalar& b) const;

    const Underlying& GetUnderlying() const;
    bool IsValid() const;
    bool IsZero() const;

    MclScalar Invert() const;
    MclScalar Negate() const;
    MclScalar Square() const;
    MclScalar Cube() const;
    MclScalar Pow(const MclScalar& n) const;

    static MclScalar Rand(const bool exclude_zero = true);

    uint64_t GetUint64() const;

    std::vector<uint8_t> GetVch(const bool trim_preceeding_zeros = false) const;
    void SetVch(const std::vector<uint8_t>& v);

    /**
     * Sets 2^n to the instance
     */
    void SetPow2(const uint32_t& n);

    uint256 GetHashWithSalt(const uint64_t& salt) const;

    std::string GetString(const int8_t& radix = 16) const;

    /**
     * extracts a specified bit of 32-byte serialization result
     */
    bool GetSeriBit(const uint8_t& n) const;

    /**
     * returns the binary representation of m_scalar
     */
    std::vector<bool> ToBinaryVec() const;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        auto vec = GetVch();
        s.write(MakeByteSpan(vec));
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        std::vector<unsigned char> vec(SERIALIZATION_SIZE);
        s.read(MakeWritableByteSpan(vec));
        SetVch(vec);
    }

    static constexpr int SERIALIZATION_SIZE = 32;

    Underlying m_scalar;
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_MCL_SCALAR_H
