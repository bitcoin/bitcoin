// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include <assert.h>
#include <cstring>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>
#include "crypto/common.h"
#include "base_uint.h"


/** 160-bit unsigned big integer. */
class uint160 : public base_uint<160> {
public:
    uint160() {}
    uint160(const base_uint<160>& b) : base_uint<160>(b) {}
    explicit uint160(const std::vector<unsigned char>& vch) : base_uint<160>(vch) {}
};

/** 256-bit unsigned big integer. */
class uint256 : public base_uint<256> {
public:
    uint256() {}
    uint256(const base_uint<256>& b) : base_uint<256>(b) {}
    uint256(uint64_t b) : base_uint<256>(b) {}
    explicit uint256(const std::string& str) : base_uint<256>(str) {}
    explicit uint256(const std::vector<unsigned char>& vch) : base_uint<256>(vch) {}

    /**
    * The "compact" format is a representation of a whole
    * number N using an unsigned 32bit number similar to a
    * floating point format.
    * The most significant 8 bits are the unsigned exponent of base 256.
    * This exponent can be thought of as "number of bytes of N".
    * The lower 23 bits are the mantissa.
    * Bit number 24 (0x800000) represents the sign of N.
    * N = (-1^sign) * mantissa * 256^(exponent-3)
    *
    * Satoshi's original implementation used BN_bn2mpi() and BN_mpi2bn().
    * MPI uses the most significant bit of the first byte as sign.
    * Thus 0x1234560000 is compact (0x05123456)
    * and  0xc0de000000 is compact (0x0600c0de)
    *
    * Bitcoin only uses this "compact" format for encoding difficulty
    * targets, which are unsigned 256bit quantities.  Thus, all the
    * complexities of the sign bit and using base 256 are probably an
    * implementation accident.
    */
    uint256& SetCompact(uint32_t nCompact, bool *pfNegative = NULL, bool *pfOverflow = NULL);
    uint32_t GetCompact(bool fNegative = false) const;

    /** A cheap hash function that just returns 64 bits from the result, it can be
     * used when the contents are considered uniformly random. It is not appropriate
     * when the value can easily be influenced from outside as e.g. a network adversary could
     * provide values to trigger worst-case behavior.
     */
    uint64_t GetCheapHash() const
    {
        return ReadLE64(reinterpret_cast<const unsigned char*>(pn));
    }
};

// alias
typedef uint256 arith_uint256;
inline uint256 UintToArith256(const uint256& u) { return u; }
inline uint256 ArithToUint256(const uint256& u) { return u; }

/* uint256 from const char *.
 * This is a separate function because the constructor uint256(const char*) can result
 * in dangerously catching uint256(0).
 */
inline uint256 uint256S(const char *str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}
/* uint256 from std::string.
 * This is a separate function because the constructor uint256(const std::string &str) can result
 * in dangerously catching uint256(0) via std::string(const char*).
 */
inline uint256 uint256S(const std::string& str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}

#endif // BITCOIN_UINT256_H
