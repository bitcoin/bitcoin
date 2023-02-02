// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_BLSCT_ARITH_MCL_MCL_SCALAR_H
#define NAVCOIN_BLSCT_ARITH_MCL_MCL_SCALAR_H

#include <functional>
#include <stddef.h>
#include <string>
#include <vector>
#include <array>

#include <bls/bls384_256.h>
#include <blsct/arith/mcl/mcl_initializer.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>

class MclScalar
{
public:
    MclScalar(const int64_t& n = 0);
    MclScalar(const std::vector<uint8_t>& v);
    template <size_t L> MclScalar(const std::array<uint8_t,L>& a);
    MclScalar(const mclBnFr& n_fr);
    MclScalar(const uint256& n);
    MclScalar(const std::string& s, int radix);

    static void Init();

    MclScalar ApplyBitwiseOp(const MclScalar& a, const MclScalar& b,
                          std::function<uint8_t(uint8_t, uint8_t)> op) const;

    void operator=(const int64_t& n);  // using int64_t instead of uint64_t since underlying mcl lib takes int64_t

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

    mclBnFr Underlying() const;
    bool IsValid() const;
    bool IsZero() const;

    MclScalar Invert() const;
    MclScalar Negate() const;
    MclScalar Square() const;
    MclScalar Cube() const;
    MclScalar Pow(const MclScalar& n) const;

    static MclScalar Rand(const bool exclude_zero = false);

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
     * returns the binary representation m_fr
     */
    std::vector<bool> ToBinaryVec() const;

    unsigned int GetSerializeSize() const;

    template <typename Stream>
    void Serialize(Stream& s) const;

    template <typename Stream>
    void Unserialize(Stream& s);

    static constexpr int SERIALIZATION_SIZE = 32;

    using UnderlyingType = mclBnFr;
    UnderlyingType m_fr;
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_MCL_SCALAR_H
