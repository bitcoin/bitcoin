// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_MCL_MCL_G1POINT_H
#define NAVCOIN_BLSCT_ARITH_MCL_MCL_G1POINT_H

#define BLS_ETH 1

#include <bls/bls384_256.h>
#include <blsct/arith/endianness.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <uint256.h>

#include <stddef.h>
#include <string>
#include <vector>

class MclG1Point
{
public:
    using Underlying = mclBnG1;
    using Scalar = MclScalar;

    MclG1Point();
    MclG1Point(const std::vector<uint8_t>& v);
    MclG1Point(const uint256& n);
    MclG1Point(const Underlying& p);

    MclG1Point operator=(const Underlying& rhs);
    MclG1Point operator+(const MclG1Point& rhs) const;
    MclG1Point operator-(const MclG1Point& rhs) const;
    MclG1Point operator*(const Scalar& rhs) const;

    /**
     * Because  Elements cannot be used here, std::vector is used instead
     */
    std::vector<MclG1Point> operator*(const std::vector<Scalar>& ss) const;

    bool operator==(const MclG1Point& rhs) const;
    bool operator!=(const MclG1Point& rhs) const;

    MclG1Point Double() const;
    const Underlying& GetUnderlying() const;

    static MclG1Point GetBasePoint();
    static MclG1Point MapToPoint(const std::vector<uint8_t>& vec, const Endianness e = Endianness::Little);
    static MclG1Point MapToPoint(const std::string& s, const Endianness e = Endianness::Little);
    static MclG1Point HashAndMap(const std::vector<uint8_t>& vec);
    static MclG1Point Rand();

    bool IsValid() const;
    bool IsZero() const;

    std::vector<uint8_t> GetVch() const;
    bool SetVch(const std::vector<uint8_t>& vec);

    std::string GetString(const uint8_t& radix = 16) const;
    Scalar GetHashWithSalt(const uint64_t salt) const;

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

    Underlying m_point;

    static constexpr int SERIALIZATION_SIZE = 384 / 8;
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_MCL_G1POINT_H
