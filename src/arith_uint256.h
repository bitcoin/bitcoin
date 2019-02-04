// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ARITH_UINT256_H
#define BITCOIN_ARITH_UINT256_H

#include <array>
#include <assert.h>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

class uint256;

class uint_error : public std::runtime_error {
public:
    explicit uint_error(const std::string& str) : std::runtime_error(str) {}
};

/** Template base class for unsigned big integers. */
template<unsigned int BITS>
class base_uint
{
protected:
    static constexpr int WIDTH = BITS / 32;
    using data_type = std::array<uint32_t, WIDTH>;
    std::shared_ptr<data_type> data;
private:
    void copy()
    {
        if (data.use_count() > 1) {
            auto tmp = data;
            data = std::make_shared<data_type>();
            std::copy(tmp->begin(), tmp->end(), data->begin());
        }
    }
public:

    base_uint() : data(std::make_shared<data_type>())
    {
        static_assert(WIDTH > 0 && BITS % 32 == 0, "Template parameter BITS must be a positive multiple of 32.");

        data->fill(0);
    }

    base_uint(base_uint&&) = default;
    base_uint(const base_uint&) = default;

    base_uint& operator=(base_uint&&) = default;
    base_uint& operator=(const base_uint&) = default;

    base_uint(uint64_t b) : data(std::make_shared<data_type>())
    {
        static_assert(WIDTH > 0 && BITS % 32 == 0, "Template parameter BITS must be a positive multiple of 32.");

        auto& pn = *data;
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    explicit base_uint(const std::string& str);

    const base_uint operator~() const
    {
        base_uint ret;
        auto& pn = *data;
        for (int i = 0; i < WIDTH; i++)
            (*ret.data)[i] = ~pn[i];
        return ret;
    }

    const base_uint operator-() const
    {
        base_uint ret = ~(*this);
        ++ret;
        return ret;
    }

    double getdouble() const;

    base_uint& operator=(uint64_t b)
    {
        copy();
        auto& pn = *data;
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator^=(const base_uint& b)
    {
        copy();
        auto& pn = *data;
        for (int i = 0; i < WIDTH; i++)
            pn[i] ^= (*b.data)[i];
        return *this;
    }

    base_uint& operator&=(const base_uint& b)
    {
        copy();
        auto& pn = *data;
        for (int i = 0; i < WIDTH; i++)
            pn[i] &= (*b.data)[i];
        return *this;
    }

    base_uint& operator|=(const base_uint& b)
    {
        copy();
        auto& pn = *data;
        for (int i = 0; i < WIDTH; i++)
            pn[i] |= (*b.data)[i];
        return *this;
    }

    base_uint& operator^=(uint64_t b)
    {
        copy();
        auto& pn = *data;
        pn[0] ^= (unsigned int)b;
        pn[1] ^= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator|=(uint64_t b)
    {
        copy();
        auto& pn = *data;
        pn[0] |= (unsigned int)b;
        pn[1] |= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator<<=(unsigned int shift);
    base_uint& operator>>=(unsigned int shift);

    base_uint& operator+=(const base_uint& b)
    {
        copy();
        auto& pn = *data;
        uint64_t carry = 0;
        for (int i = 0; i < WIDTH; i++)
        {
            uint64_t n = carry + pn[i] + (*b.data)[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_uint& operator-=(const base_uint& b)
    {
        *this += -b;
        return *this;
    }

    base_uint& operator+=(uint64_t b64)
    {
        *this += base_uint(b64);
        return *this;
    }

    base_uint& operator-=(uint64_t b64)
    {
        *this += -base_uint(b64);
        return *this;
    }

    base_uint& operator*=(uint32_t b32);
    base_uint& operator*=(const base_uint& b);
    base_uint& operator/=(const base_uint& b);

    base_uint& operator++()
    {
        // prefix operator
        copy();
        int i = 0;
        auto& pn = *data;
        while (i < WIDTH && ++pn[i] == 0)
            i++;
        return *this;
    }

    const base_uint operator++(int)
    {
        // postfix operator
        const base_uint ret = *this;
        ++(*this);
        return ret;
    }

    base_uint& operator--()
    {
        // prefix operator
        copy();
        int i = 0;
        auto& pn = *data;
        while (i < WIDTH && --pn[i] == std::numeric_limits<uint32_t>::max())
            i++;
        return *this;
    }

    const base_uint operator--(int)
    {
        // postfix operator
        const base_uint ret = *this;
        --(*this);
        return ret;
    }

    int CompareTo(const base_uint& b) const;
    bool EqualTo(uint64_t b) const;

    friend inline const base_uint operator+(const base_uint& a, const base_uint& b) { return base_uint(a) += b; }
    friend inline const base_uint operator-(const base_uint& a, const base_uint& b) { return base_uint(a) -= b; }
    friend inline const base_uint operator*(const base_uint& a, const base_uint& b) { return base_uint(a) *= b; }
    friend inline const base_uint operator/(const base_uint& a, const base_uint& b) { return base_uint(a) /= b; }
    friend inline const base_uint operator|(const base_uint& a, const base_uint& b) { return base_uint(a) |= b; }
    friend inline const base_uint operator&(const base_uint& a, const base_uint& b) { return base_uint(a) &= b; }
    friend inline const base_uint operator^(const base_uint& a, const base_uint& b) { return base_uint(a) ^= b; }
    friend inline const base_uint operator>>(const base_uint& a, int shift) { return base_uint(a) >>= shift; }
    friend inline const base_uint operator<<(const base_uint& a, int shift) { return base_uint(a) <<= shift; }
    friend inline const base_uint operator*(const base_uint& a, uint32_t b) { return base_uint(a) *= b; }
    friend inline bool operator==(const base_uint& a, const base_uint& b) { return memcmp(a.data->begin(), b.data->begin(), a.size()) == 0; }
    friend inline bool operator!=(const base_uint& a, const base_uint& b) { return !(a == b); }
    friend inline bool operator>(const base_uint& a, const base_uint& b) { return a.CompareTo(b) > 0; }
    friend inline bool operator<(const base_uint& a, const base_uint& b) { return a.CompareTo(b) < 0; }
    friend inline bool operator>=(const base_uint& a, const base_uint& b) { return a.CompareTo(b) >= 0; }
    friend inline bool operator<=(const base_uint& a, const base_uint& b) { return a.CompareTo(b) <= 0; }
    friend inline bool operator==(const base_uint& a, uint64_t b) { return a.EqualTo(b); }
    friend inline bool operator!=(const base_uint& a, uint64_t b) { return !a.EqualTo(b); }

    std::string GetHex() const;
    void SetHex(const char* psz);
    void SetHex(const std::string& str);
    std::string ToString() const;

    constexpr std::size_t size() const
    {
        return WIDTH * sizeof(uint32_t);
    }

    /**
     * Returns the position of the highest bit set plus one, or zero if the
     * value is zero.
     */
    unsigned int bits() const;

    uint64_t GetLow64() const
    {
        auto& pn = *data;
        static_assert(WIDTH >= 2, "Assertion size >= 2 failed (size = BITS / 32). BITS is a template parameter.");
        return pn[0] | (uint64_t)pn[1] << 32;
    }
};

/** 256-bit unsigned big integer. */
class arith_uint256 : public base_uint<256> {
public:
    arith_uint256() = default;
    using base_uint<256>::base_uint;
    arith_uint256(arith_uint256&&) = default;
    arith_uint256(const arith_uint256&) = default;
    arith_uint256& operator=(arith_uint256&&) = default;
    arith_uint256& operator=(const arith_uint256&) = default;

    arith_uint256(base_uint<256>&& b) : base_uint<256>(std::move(b)) {}
    arith_uint256(const base_uint<256>& b) : base_uint<256>(b) {}

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
    arith_uint256& SetCompact(uint32_t nCompact, bool *pfNegative = nullptr, bool *pfOverflow = nullptr);
    uint32_t GetCompact(bool fNegative = false) const;

    friend uint256 ArithToUint256(const arith_uint256 &);
    friend arith_uint256 UintToArith256(const uint256 &);
};

uint256 ArithToUint256(const arith_uint256 &);
arith_uint256 UintToArith256(const uint256 &);

#endif // BITCOIN_ARITH_UINT256_H
