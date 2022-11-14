// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_BLSCT_ARITH_SCALAR_H
#define NAVCOIN_BLSCT_ARITH_SCALAR_H

#include <functional>
#include <stddef.h>
#include <string>
#include <vector>

#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

#include <bls/bls384_256.h> // must include this before bls/bls.h
#include <bls/bls.h>
#include <blsct/arith/mcl_initializer.h>
#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>

class Scalar {
public:
    static constexpr int SERIALIZATION_SIZE_IN_BYTES = 32;

    Scalar(const int64_t& n = 0);  // has to take int64_t instead of uint64_t since underneath it calls mcl library that takes int64_t
    Scalar(const std::vector<uint8_t>& v);
    Scalar(const mclBnFr& n_fr);
    Scalar(const uint256& n);
    Scalar(const std::string& s, int radix);

    static void Init();

    Scalar ApplyBitwiseOp(const Scalar& a, const Scalar& b,
                          std::function<uint8_t(uint8_t, uint8_t)> op) const;

    void operator=(const int64_t& n);  // using int64_t instead of uint64_t since underlying mcl lib takes int64_t

    Scalar operator+(const Scalar& b) const;
    Scalar operator-(const Scalar& b) const;
    Scalar operator*(const Scalar& b) const;
    Scalar operator/(const Scalar& b) const;
    Scalar operator|(const Scalar& b) const;
    Scalar operator^(const Scalar& b) const;
    Scalar operator&(const Scalar& b) const;
    Scalar operator~() const;
    Scalar operator<<(const uint32_t& shift) const;
    Scalar operator>>(const uint32_t& shift) const;

    bool operator==(const Scalar& b) const;
    bool operator==(const int32_t& b) const;
    bool operator!=(const Scalar& b) const;
    bool operator!=(const int32_t& b) const;

    bool IsValid() const;

    Scalar Invert() const;
    Scalar Negate() const;
    Scalar Square() const;
    Scalar Cube() const;
    Scalar Pow(const Scalar& n) const;

    static Scalar Rand(const bool exclude_zero = false);

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
    void Serialize(Stream& s) const
    {
        ::Serialize(s, GetVch());
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        std::vector<uint8_t> vch;
        ::Unserialize(s, vch);
        SetVch(vch);
    }

    mclBnFr m_fr;
};

#endif // NAVCOIN_BLSCT_ARITH_SCALAR_H
