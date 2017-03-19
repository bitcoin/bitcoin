// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <limits>
#include <vector>

class scriptnum_error : public std::runtime_error
{
public:
    explicit scriptnum_error(const std::string& str) : std::runtime_error(str) {}
};

class CScriptNum
{
    /**
    * Numeric opcodes (OP_1ADD, etc) are restricted to operating on 4-byte integers.
    * The semantics are subtle, though: operands must be in the range [-2^31 +1...2^31 -1],
    * but results may overflow (and are valid as long as they are not used in a subsequent
    * numeric operation). CScriptNum enforces those semantics by storing results as
    * an int64 and allowing out-of-range values to be returned as a vector of bytes but
    * throwing an exception if arithmetic is done or the result is interpreted as an integer.
    */
public:

    explicit CScriptNum(const int64_t& n)
    {
        m_value = n;
    }

    static const size_t nDefaultMaxNumSize = 4;

    explicit CScriptNum(const std::vector<unsigned char>& vch, bool fRequireMinimal,
        const size_t nMaxNumSize = nDefaultMaxNumSize)
    {
        if (vch.size() > nMaxNumSize) {
            throw scriptnum_error("script number overflow");
        }
        if (fRequireMinimal && vch.size() > 0) {
            // Check that the number is encoded with the minimum possible
            // number of bytes.
            //
            // If the most-significant-byte - excluding the sign bit - is zero
            // then we're not minimal. Note how this test also rejects the
            // negative-zero encoding, 0x80.
            if ((vch.back() & 0x7f) == 0) {
                // One exception: if there's more than one byte and the most
                // significant bit of the second-most-significant-byte is set
                // it would conflict with the sign bit. An example of this case
                // is +-255, which encode to 0xff00 and 0xff80 respectively.
                // (big-endian).
                if (vch.size() <= 1 || (vch[vch.size() - 2] & 0x80) == 0) {
                    throw scriptnum_error("non-minimally encoded script number");
                }
            }
        }
        m_value = set_vch(vch);
    }

    inline bool operator==(const int64_t& rhs) const { return m_value == rhs; }
    inline bool operator!=(const int64_t& rhs) const { return m_value != rhs; }
    inline bool operator<=(const int64_t& rhs) const { return m_value <= rhs; }
    inline bool operator< (const int64_t& rhs) const { return m_value <  rhs; }
    inline bool operator>=(const int64_t& rhs) const { return m_value >= rhs; }
    inline bool operator> (const int64_t& rhs) const { return m_value >  rhs; }

    inline bool operator==(const CScriptNum& rhs) const { return operator==(rhs.m_value); }
    inline bool operator!=(const CScriptNum& rhs) const { return operator!=(rhs.m_value); }
    inline bool operator<=(const CScriptNum& rhs) const { return operator<=(rhs.m_value); }
    inline bool operator< (const CScriptNum& rhs) const { return operator< (rhs.m_value); }
    inline bool operator>=(const CScriptNum& rhs) const { return operator>=(rhs.m_value); }
    inline bool operator> (const CScriptNum& rhs) const { return operator> (rhs.m_value); }

    inline CScriptNum operator+(const int64_t& rhs)    const { return CScriptNum(m_value + rhs); }
    inline CScriptNum operator-(const int64_t& rhs)    const { return CScriptNum(m_value - rhs); }
    inline CScriptNum operator+(const CScriptNum& rhs) const { return operator+(rhs.m_value); }
    inline CScriptNum operator-(const CScriptNum& rhs) const { return operator-(rhs.m_value); }

    inline CScriptNum& operator+=(const CScriptNum& rhs) { return operator+=(rhs.m_value); }
    inline CScriptNum& operator-=(const CScriptNum& rhs) { return operator-=(rhs.m_value); }

    inline CScriptNum operator&(const int64_t& rhs)    const { return CScriptNum(m_value & rhs); }
    inline CScriptNum operator&(const CScriptNum& rhs) const { return operator&(rhs.m_value); }

    inline CScriptNum& operator&=(const CScriptNum& rhs) { return operator&=(rhs.m_value); }

    inline CScriptNum operator-()                         const
    {
        assert(m_value != std::numeric_limits<int64_t>::min());
        return CScriptNum(-m_value);
    }

    inline CScriptNum& operator=(const int64_t& rhs)
    {
        m_value = rhs;
        return *this;
    }

    inline CScriptNum& operator+=(const int64_t& rhs)
    {
        assert(rhs == 0 || (rhs > 0 && m_value <= std::numeric_limits<int64_t>::max() - rhs) ||
            (rhs < 0 && m_value >= std::numeric_limits<int64_t>::min() - rhs));
        m_value += rhs;
        return *this;
    }

    inline CScriptNum& operator-=(const int64_t& rhs)
    {
        assert(rhs == 0 || (rhs > 0 && m_value >= std::numeric_limits<int64_t>::min() + rhs) ||
            (rhs < 0 && m_value <= std::numeric_limits<int64_t>::max() + rhs));
        m_value -= rhs;
        return *this;
    }

    inline CScriptNum& operator&=(const int64_t& rhs)
    {
        m_value &= rhs;
        return *this;
    }

    // #### 割と無理やり変換している.
    int getint() const
    {
        if (m_value > std::numeric_limits<int>::max())
            return std::numeric_limits<int>::max();
        else if (m_value < std::numeric_limits<int>::min())
            return std::numeric_limits<int>::min();
        return (int)m_value;
    }

    std::vector<unsigned char> getvch() const
    {
        return serialize(m_value);
    }

    static std::vector<unsigned char> serialize(const int64_t& value)
    {
        if (value == 0)
            return std::vector<unsigned char>();

        std::vector<unsigned char> result;
        const bool neg = value < 0;
        uint64_t absvalue = neg ? -value : value;

        while (absvalue)
        {
            result.push_back(absvalue & 0xff);
            absvalue >>= 8;
        }

        //    - If the most significant byte is >= 0x80 and the value is positive, push a
        //    new zero-byte to make the significant byte < 0x80 again.

        //    - If the most significant byte is >= 0x80 and the value is negative, push a
        //    new 0x80 byte that will be popped off when converting to an integral.

        //    - If the most significant byte is < 0x80 and the value is negative, add
        //    0x80 to it, since it will be subtracted and interpreted as a negative when
        //    converting to an integral.

        if (result.back() & 0x80)
            result.push_back(neg ? 0x80 : 0);
        else if (neg)
            result.back() |= 0x80;

        return result;
    }

private:
    static int64_t set_vch(const std::vector<unsigned char>& vch)
    {
        if (vch.empty())
            return 0;

        int64_t result = 0;
        for (size_t i = 0; i != vch.size(); ++i)
            result |= static_cast<int64_t>(vch[i]) << 8 * i;

        // If the input vector's most significant byte is 0x80, remove it from
        // the result's msb and return a negative.
        if (vch.back() & 0x80)
            return -((int64_t)(result & ~(0x80ULL << (8 * (vch.size() - 1)))));

        return result;
    }

    int64_t m_value;
};
