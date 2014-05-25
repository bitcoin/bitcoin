// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MONEY_H
#define BITCOIN_MONEY_H

#include "serialize.h"

#include <algorithm> // for swap
#include <stdint.h>
#include <stdexcept>
#include <string>

enum RoundingMode
{
    ROUND_TIES_TO_EVEN,
    ROUND_TOWARDS_ZERO,
    ROUND_AWAY_FROM_ZERO,
    ROUND_TOWARD_POSITIVE,
    ROUND_TOWARD_NEGATIVE,
    ROUND_SIGNAL,
};

class money_error : public std::runtime_error
{
public:
    explicit money_error(const std::string& str)
        : std::runtime_error(str) {}
};

class invalid_money_format : public money_error
{
public:
    explicit invalid_money_format(const std::string& str)
        : money_error(str) {}
};

class money_overflow : public money_error
                     , public std::overflow_error
{
public:
    explicit money_overflow(const std::string& str)
        : money_error(str)
        , std::overflow_error(str) {}
};

class money_underflow : public money_error
                      , public std::underflow_error
{
public:
    explicit money_underflow(const std::string& str)
        : money_error(str)
        , std::underflow_error(str) {}
};

class CMoney
{
protected:
    int64_t n;

public:
    CMoney() : n(0) {}
    CMoney(int64_t nIn) : n(nIn) {}
    CMoney(const CMoney& other) : n(other.n) {}

    CMoney& operator=(const CMoney& other)
    {
        n = other.n;
        return *this;
    }

    CMoney& swap(CMoney& other)
    {
        std::swap(n, other.n);
        return *this;
    }

    // In units of 1 satoshi
    double ToDouble(RoundingMode mode=ROUND_TIES_TO_EVEN) const
    {
        return static_cast<double>(n);
    }

    // In units of 1 satoshi
    int64_t ToInt64(RoundingMode mode=ROUND_TIES_TO_EVEN) const
    {
        return n;
    }

    CMoney& operator+=(const CMoney& other)
    {
        n += other.n;
        return *this;
    }

    CMoney& operator-=(const CMoney& other)
    {
        n -= other.n;
        return *this;
    }

    CMoney& operator*=(int64_t other)
    {
        n *= other;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream &o, const CMoney& n);

    friend bool operator! (const CMoney& a);
    friend bool operator< (const CMoney& a, const CMoney& b);
    friend bool operator<=(const CMoney& a, const CMoney& b);
    friend bool operator==(const CMoney& a, const CMoney& b);
    friend bool operator!=(const CMoney& a, const CMoney& b);
    friend bool operator>=(const CMoney& a, const CMoney& b);
    friend bool operator> (const CMoney& a, const CMoney& b);

    friend const CMoney operator-(const CMoney& a);
    friend const CMoney operator+(const CMoney& a, const CMoney& b);
    friend const CMoney operator-(const CMoney& a, const CMoney& b);
    friend const CMoney operator*(int64_t a, const CMoney& b);
    friend const CMoney operator*(const CMoney& a, int64_t b);
    friend const CMoney abs(const CMoney& a);
};

namespace std {
template<> inline CMoney numeric_limits<CMoney>::min()
    { return CMoney(std::numeric_limits<int64_t>::min()); }
template<> inline CMoney numeric_limits<CMoney>::max()
    { return CMoney(std::numeric_limits<int64_t>::max()); }
}

// Required by use of Boost unit testing libraries
std::string FormatMoney(const CMoney& a, bool fPlus);
inline std::ostream& operator<<(std::ostream &o, const CMoney& n)
{
    o << FormatMoney(n, false);
    return o;
}

inline bool operator!(const CMoney& a)
{
    return a.n == 0;
}

inline bool operator<(const CMoney& a, const CMoney& b)
{
    return a.n < b.n;
}

inline bool operator<=(const CMoney& a, const CMoney& b)
{
    return a.n <= b.n;
}

inline bool operator==(const CMoney& a, const CMoney& b)
{
    return a.n == b.n;
}

inline bool operator!=(const CMoney& a, const CMoney& b)
{
    return a.n != b.n;
}

inline bool operator>=(const CMoney& a, const CMoney& b)
{
    return a.n >= b.n;
}

inline bool operator>(const CMoney& a, const CMoney& b)
{
    return a.n > b.n;
}

inline const CMoney operator-(const CMoney& a)
{
    return CMoney(-a.n);
}

inline const CMoney operator+(const CMoney& a, const CMoney& b)
{
    return CMoney(a.n + b.n);
}

inline const CMoney operator-(const CMoney& a, const CMoney& b)
{
    return CMoney(a.n - b.n);
}

inline const CMoney operator*(int64_t a, const CMoney& b)
{
    return CMoney(a * b.n);
}

inline const CMoney operator*(const CMoney& a, int64_t b)
{
    return CMoney(a.n * b);
}

inline const CMoney abs(const CMoney& a)
{
    return CMoney(std::abs(a.n));
}

class CCompressedMoney
{
protected:
    CMoney& n;

public:
    static uint64_t CompressAmount(uint64_t nAmount);
    static uint64_t DecompressAmount(uint64_t nAmount);

    CCompressedMoney(CMoney& nIn) : n(nIn) {}

    IMPLEMENT_SERIALIZE(
        uint64_t nVal = 0;
        if (fWrite)
            nVal = CompressAmount(static_cast<uint64_t>(n.ToInt64(ROUND_SIGNAL)));
        READWRITE(VARINT(nVal));
        if (fRead)
            n = static_cast<int64_t>(DecompressAmount(nVal));
    )
};

#endif
