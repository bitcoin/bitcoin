// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_G1POINT_H
#define NAVCOIN_BLSCT_ARITH_G1POINT_H

#include <stddef.h>
#include <string>

#include <bls/bls384_256.h> // must include this before bls/bls.h
#include <bls/bls.h>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

#include <blsct/arith/mcl_initializer.h>
#include <blsct/arith/scalar.h>
#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>

enum class Endianness {
    Big,
    Little
};

class G1Point
{
public:
    static constexpr int WIDTH = 384 / 8;

    G1Point();
    G1Point(const std::vector<uint8_t>& v);
    G1Point(const uint256& b);
    G1Point(const mclBnG1& p);

    static void Init();

    G1Point operator=(const mclBnG1& p);
    G1Point operator+(const G1Point& b) const;
    G1Point operator-(const G1Point& b) const;
    G1Point operator*(const Scalar& b) const;

    G1Point Double() const;

    static G1Point GetBasePoint();
    static G1Point MapToG1(const std::vector<uint8_t>& vec, const Endianness e = Endianness::Little);
    static G1Point MapToG1(const std::string& s, const Endianness e = Endianness::Little);
    static G1Point HashAndMap(const std::vector<uint8_t>& vec);

    /**
     * Multiply G1Points by Scalars element by element and then get the sum of all resulting points
     * [g_1*s_1, g_2*s_2, ..., g_n*s_n].Sum()
     */
    static G1Point MulVec(const std::vector<mclBnG1>& g_vec, const std::vector<mclBnFr>& s_vec);

    static G1Point Rand();

    bool operator==(const G1Point& b) const;
    bool operator!=(const G1Point& b) const;

    bool IsUnity() const;

    std::vector<uint8_t> GetVch() const;
    void SetVch(const std::vector<uint8_t>& b);

    std::string GetString(const int& radix = 16) const;

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

    mclBnG1 m_p;

private:
    static mclBnG1 m_g; // Using mclBnG1 instead of G1Point to get around chiken-and-egg issue
    static boost::mutex m_init_mutex;
};

#endif // NAVCOIN_BLSCT_ARITH_G1POINT_H
