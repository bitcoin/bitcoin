// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_BLSCT_SCALAR_H
#define NAVCOIN_BLSCT_SCALAR_H

#ifdef _WIN32
/* Avoid redefinition warning. */
#undef ERROR
#undef WSIZE
#undef DOUBLE
#endif

#include <bls.hpp>
#include <hash.h>
#include <uint256.h>
#include <serialize.h>
#include <version.h>

#include "relic_conf.h"

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

#include "relic.h"
#include "relic_bn.h"
#include "relic_core.h"
#include "relic_test.h"

#include <stddef.h>
#include <string>
#include <vector>

#define CHECK_AND_ASSERT_THROW_MES(expr, message) do {if(!(expr)) throw std::runtime_error(message);} while(0)

class Scalar {
public:
    Scalar();
    Scalar(const uint64_t& n);
    Scalar(const bls::PrivateKey& n);
    Scalar(const std::vector<uint8_t> &v);
    Scalar(const bn_t &b);
    Scalar(const Scalar& n);
    Scalar(const uint256& n);

    void operator=(const uint64_t& n);

    Scalar operator+(const Scalar &b) const;
    Scalar operator-(const Scalar &b) const;
    Scalar operator*(const Scalar &b) const;
    Scalar operator|(const Scalar &b) const;
    Scalar operator^(const Scalar &b) const;
    Scalar operator&(const Scalar &b) const;
    Scalar operator~() const;
    Scalar operator<<(const int &b) const;
    Scalar operator>>(const int &b) const;

    bool operator==(const Scalar& b) const;
    bool operator==(const int &b) const;

    Scalar Invert() const;
    Scalar Negate() const;

    bool GetBit(size_t n) const;
    int64_t GetInt64() const;

    std::vector<uint8_t> GetVch() const;
    void SetVch(const std::vector<uint8_t>& b);

    void SetPow2(const int& n);

    uint256 Hash(const int& n) const;

    static Scalar Rand();

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(GetVch(), nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, GetVch(), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        std::vector<uint8_t> vch;
        ::Unserialize(s, vch, nType, nVersion);
        SetVch(vch);
    }

    bn_t bn;
};

uint256 HashG1Element(bls::G1Element g1, uint64_t n);

#endif // NAVCOIN_BLSCT_SCALAR_H
