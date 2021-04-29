/*
uint256_t.h
An unsigned 256 bit integer library for C++

Copyright (c) 2013 - 2017 Jason Lee @ calccrypto at gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

With much help from Auston Sterling

Thanks to François Dessenne for convincing me
to do a general rewrite of this class.
*/

#ifndef __UINT256_T__
#define __UINT256_T__

#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

class UINT256_T_EXTERN uint256_t;

// Give uint256_t type traits
namespace std {  // This is probably not a good idea
    template <> struct is_arithmetic <uint256_t> : std::true_type {};
    template <> struct is_integral   <uint256_t> : std::true_type {};
    template <> struct is_unsigned   <uint256_t> : std::true_type {};
}

class uint256_t{
    private:
#ifdef __BIG_ENDIAN__
        uint128_t UPPER, LOWER;
#endif
#ifdef __LITTLE_ENDIAN__
        uint128_t LOWER, UPPER;
#endif

    public:
        // Constructors
        uint256_t() = default;
        uint256_t(const uint256_t & rhs) = default;
        uint256_t(uint256_t && rhs) = default;
        uint256_t(const std::string & s);
        uint256_t(const char *val);
        uint256_t(const std::string & s, uint8_t base);
        uint256_t(const char *val, uint8_t base);
        uint256_t(const bool & b);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t(const T & rhs)
#ifdef __BIG_ENDIAN__
            : UPPER(uint128_0), LOWER(rhs)
#endif
#ifdef __LITTLE_ENDIAN__
            : LOWER(rhs), UPPER(uint128_0)
#endif
        {
            if (std::is_signed<T>::value) {
                if (rhs < 0) {
                    UPPER = uint128_t(-1, -1);
                }
            }
        }

        template <typename S, typename T, typename = typename std::enable_if <std::is_integral<S>::value && std::is_integral<T>::value, void>::type>
        uint256_t(const S & upper_rhs, const T & lower_rhs)
#ifdef __BIG_ENDIAN__
            : UPPER(upper_rhs), LOWER(lower_rhs)
#endif
#ifdef __LITTLE_ENDIAN__
            : LOWER(lower_rhs), UPPER(upper_rhs)
#endif
        {}

        uint256_t(const uint128_t & upper_rhs, const uint128_t & lower_rhs)
#ifdef __BIG_ENDIAN__
            : UPPER(upper_rhs), LOWER(lower_rhs)
#endif
#ifdef __LITTLE_ENDIAN__
            : LOWER(lower_rhs), UPPER(upper_rhs)
#endif
        {}
        uint256_t(const uint128_t & lower_rhs)
#ifdef __BIG_ENDIAN__
            : UPPER(uint128_0), LOWER(lower_rhs)
#endif
#ifdef __LITTLE_ENDIAN__
            : LOWER(lower_rhs), UPPER(uint128_0)
#endif
        {}

       template <typename R, typename S, typename T, typename U,
                typename = typename std::enable_if<std::is_integral<R>::value &&
                std::is_integral<S>::value &&
                std::is_integral<T>::value &&
                std::is_integral<U>::value, void>::type>
        uint256_t(const R & upper_lhs, const S & lower_lhs, const T & upper_rhs, const U & lower_rhs)
#ifdef __BIG_ENDIAN__
            : UPPER(upper_lhs, lower_lhs), LOWER(upper_rhs, lower_rhs)
#endif
#ifdef __LITTLE_ENDIAN__
            : LOWER(upper_rhs, lower_rhs), UPPER(upper_lhs, lower_lhs)
#endif
        {}

        //  RHS input args only
		std::vector<uint8_t> export_bits() const;
        std::vector<uint8_t> export_bits_truncate() const;

        // Assignment Operator
        uint256_t & operator=(const uint256_t & rhs) = default;
        uint256_t & operator=(uint256_t && rhs) = default;

        template <typename T, typename = typename std::enable_if <std::is_integral<T>::value, T>::type>
        uint256_t & operator=(const T & rhs){
            UPPER = uint128_0;

            if (std::is_signed<T>::value) {
                if (rhs < 0) {
                    UPPER = uint128_t(-1, -1);
                }
            }

            LOWER = rhs;
            return *this;
        }

        uint256_t & operator=(const bool & rhs);

        // Typecast Operators
        operator bool      () const;
        operator uint8_t   () const;
        operator uint16_t  () const;
        operator uint32_t  () const;
        operator uint64_t  () const;
        operator uint128_t () const;

        // Bitwise Operators
        uint256_t operator&(const uint128_t & rhs) const;
        uint256_t operator&(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator&(const T & rhs) const{
            return uint256_t(uint128_0, LOWER & (uint128_t) rhs);
        }

        uint256_t & operator&=(const uint128_t & rhs);
        uint256_t & operator&=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator&=(const T & rhs){
            UPPER = uint128_0;
            LOWER &= rhs;
            return *this;
        }

        uint256_t operator|(const uint128_t & rhs) const;
        uint256_t operator|(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator|(const T & rhs) const{
            return uint256_t(UPPER, LOWER | uint128_t(rhs));
        }

        uint256_t & operator|=(const uint128_t & rhs);
        uint256_t & operator|=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator|=(const T & rhs){
            LOWER |= (uint128_t) rhs;
            return *this;
        }

        uint256_t operator^(const uint128_t & rhs) const;
        uint256_t operator^(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator^(const T & rhs) const{
            return uint256_t(UPPER, LOWER ^ (uint128_t) rhs);
        }

        uint256_t & operator^=(const uint128_t & rhs);
        uint256_t & operator^=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator^=(const T & rhs){
            LOWER ^= (uint128_t) rhs;
            return *this;
        }

        uint256_t operator~() const;

        // Bit Shift Operators
        uint256_t operator<<(const uint128_t & shift) const;
        uint256_t operator<<(const uint256_t & shift) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator<<(const T & rhs) const{
            return *this << uint256_t(rhs);
        }

        uint256_t & operator<<=(const uint128_t & shift);
        uint256_t & operator<<=(const uint256_t & shift);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator<<=(const T & rhs){
            *this = *this << uint256_t(rhs);
            return *this;
        }

        uint256_t operator>>(const uint128_t & shift) const;
        uint256_t operator>>(const uint256_t & shift) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator>>(const T & rhs) const{
            return *this >> uint256_t(rhs);
        }

        uint256_t & operator>>=(const uint128_t & shift);
        uint256_t & operator>>=(const uint256_t & shift);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator>>=(const T & rhs){
            *this = *this >> uint256_t(rhs);
            return *this;
        }

        // Logical Operators
        bool operator!() const;

        bool operator&&(const uint128_t & rhs) const;
        bool operator&&(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator&&(const T & rhs) const{
            return ((bool) *this && rhs);
        }

        bool operator||(const uint128_t & rhs) const;
        bool operator||(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator||(const T & rhs) const{
            return ((bool) *this || rhs);
        }

        // Comparison Operators
        bool operator==(const uint128_t & rhs) const;
        bool operator==(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator==(const T & rhs) const{
            return (!UPPER && (LOWER == uint128_t(rhs)));
        }

        bool operator!=(const uint128_t & rhs) const;
        bool operator!=(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator!=(const T & rhs) const{
            return ((bool) UPPER | (LOWER != uint128_t(rhs)));
        }

        bool operator>(const uint128_t & rhs) const;
        bool operator>(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator>(const T & rhs) const{
            return ((bool) UPPER | (LOWER > uint128_t(rhs)));
        }

        bool operator<(const uint128_t & rhs) const;
        bool operator<(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator<(const T & rhs) const{
            return (!UPPER)?(LOWER < uint128_t(rhs)):false;
        }

        bool operator>=(const uint128_t & rhs) const;
        bool operator>=(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator>=(const T & rhs) const{
            return ((*this > rhs) | (*this == rhs));
        }

        bool operator<=(const uint128_t & rhs) const;
        bool operator<=(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        bool operator<=(const T & rhs) const{
            return ((*this < rhs) | (*this == rhs));
        }

        // Arithmetic Operators
        uint256_t operator+(const uint128_t & rhs) const;
        uint256_t operator+(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator+(const T & rhs) const{
            return uint256_t(UPPER + ((LOWER + (uint128_t) rhs) < LOWER), LOWER + (uint128_t) rhs);
        }

        uint256_t & operator+=(const uint128_t & rhs);
        uint256_t & operator+=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator+=(const T & rhs){
            return *this += uint256_t(rhs);
        }

        uint256_t operator-(const uint128_t & rhs) const;
        uint256_t operator-(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator-(const T & rhs) const{
            return uint256_t(UPPER - ((LOWER - rhs) > LOWER), LOWER - rhs);
        }

        uint256_t & operator-=(const uint128_t & rhs);
        uint256_t & operator-=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator-=(const T & rhs){
            return *this = *this - uint256_t(rhs);
        }

        uint256_t operator*(const uint128_t & rhs) const;
        uint256_t operator*(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator*(const T & rhs) const{
            return *this * uint256_t(rhs);
        }

        uint256_t & operator*=(const uint128_t & rhs);
        uint256_t & operator*=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator*=(const T & rhs){
            return *this = *this * uint256_t(rhs);
        }

    private:
        std::pair <uint256_t, uint256_t> divmod(const uint256_t & lhs, const uint256_t & rhs) const;
        void init(const char * s);
        void init_from_base(const char * s, uint8_t base);

    public:
        uint256_t operator/(const uint128_t & rhs) const;
        uint256_t operator/(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator/(const T & rhs) const{
            return *this / uint256_t(rhs);
        }

        uint256_t & operator/=(const uint128_t & rhs);
        uint256_t & operator/=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator/=(const T & rhs){
            return *this = *this / uint256_t(rhs);
        }

        uint256_t operator%(const uint128_t & rhs) const;
        uint256_t operator%(const uint256_t & rhs) const;

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t operator%(const T & rhs) const{
            return *this % uint256_t(rhs);
        }

        uint256_t & operator%=(const uint128_t & rhs);
        uint256_t & operator%=(const uint256_t & rhs);

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
        uint256_t & operator%=(const T & rhs){
            return *this = *this % uint256_t(rhs);
        }

        // Increment Operators
        uint256_t & operator++();
        uint256_t operator++(int);

        // Decrement Operators
        uint256_t & operator--();
        uint256_t operator--(int);

        // Nothing done since promotion doesn't work here
        uint256_t operator+() const;

        // two's complement
        uint256_t operator-() const;

        // Get private values
        const uint128_t & upper() const;
        const uint128_t & lower() const;

        // Get bitsize of value
        uint16_t bits() const;

        // Get string representation of value
        std::string str(uint8_t base = 10, const unsigned int & len = 0) const;
};

// useful values
UINT256_T_EXTERN extern const uint128_t uint128_64;
UINT256_T_EXTERN extern const uint128_t uint128_128;
UINT256_T_EXTERN extern const uint128_t uint128_256;
UINT256_T_EXTERN extern const uint256_t uint256_0;
UINT256_T_EXTERN extern const uint256_t uint256_1;
UINT256_T_EXTERN extern const uint256_t uint256_max;

// Bitwise Operators
UINT256_T_EXTERN uint256_t operator&(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
 uint256_t operator&(const T & lhs, const uint256_t & rhs){
    return rhs & lhs;
}

UINT256_T_EXTERN uint128_t & operator&=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator&=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (rhs & lhs);
}

UINT256_T_EXTERN uint256_t operator|(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
uint256_t operator|(const T & lhs, const uint256_t & rhs){
    return rhs | lhs;
}

UINT256_T_EXTERN uint128_t & operator|=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator|=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (rhs | lhs);
}

UINT256_T_EXTERN uint256_t operator^(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
uint256_t operator^(const T & lhs, const uint256_t & rhs){
    return rhs ^ lhs;
}

uint128_t & operator^=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator^=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (rhs ^ lhs);
}

// Bitshift operators
UINT256_T_EXTERN uint256_t operator<<(const bool      & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const uint8_t   & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const uint16_t  & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const uint32_t  & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const uint64_t  & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const uint128_t & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const int8_t    & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const int16_t   & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const int32_t   & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator<<(const int64_t   & lhs, const uint256_t & rhs);

UINT256_T_EXTERN uint128_t & operator<<=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator<<=(T & lhs, const uint256_t & rhs){
    lhs = static_cast <T> (uint256_t(lhs) << rhs);
    return lhs;
}

UINT256_T_EXTERN uint256_t operator>>(const bool      & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const uint8_t   & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const uint16_t  & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const uint32_t  & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const uint64_t  & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const uint128_t & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const int8_t    & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const int16_t   & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const int32_t   & lhs, const uint256_t & rhs);
UINT256_T_EXTERN uint256_t operator>>(const int64_t   & lhs, const uint256_t & rhs);

UINT256_T_EXTERN uint128_t & operator>>=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator>>=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (uint256_t(lhs) >> rhs);
}

// Comparison Operators
UINT256_T_EXTERN bool operator==(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
bool operator==(const T & lhs, const uint256_t & rhs){
    return (!rhs.upper() && ((uint64_t) lhs == rhs.lower()));
}

UINT256_T_EXTERN bool operator!=(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
bool operator!=(const T & lhs, const uint256_t & rhs){
    return (rhs.upper() | ((uint64_t) lhs != rhs.lower()));
}

UINT256_T_EXTERN bool operator>(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
bool operator>(const T & lhs, const uint256_t & rhs){
    return rhs.upper()?false:((uint128_t) lhs > rhs.lower());
}

UINT256_T_EXTERN bool operator<(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
bool operator<(const T & lhs, const uint256_t & rhs){
    return rhs.upper()?true:((uint128_t) lhs < rhs.lower());
}

UINT256_T_EXTERN bool operator>=(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
bool operator>=(const T & lhs, const uint256_t & rhs){
    return rhs.upper()?false:((uint128_t) lhs >= rhs.lower());
}

UINT256_T_EXTERN bool operator<=(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
bool operator<=(const T & lhs, const uint256_t & rhs){
    return rhs.upper()?true:((uint128_t) lhs <= rhs.lower());
}

// Arithmetic Operators
UINT256_T_EXTERN uint256_t operator+(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
uint256_t operator+(const T & lhs, const uint256_t & rhs){
    return rhs + lhs;
}

UINT256_T_EXTERN uint128_t & operator+=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator+=(T & lhs, const uint256_t & rhs){
    lhs = static_cast <T> (rhs + lhs);
    return lhs;
}

UINT256_T_EXTERN uint256_t operator-(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
uint256_t operator-(const T & lhs, const uint256_t & rhs){
    return -(rhs - lhs);
}

UINT256_T_EXTERN uint128_t & operator-=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator-=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (-(rhs - lhs));
}

UINT256_T_EXTERN uint256_t operator*(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
uint256_t operator*(const T & lhs, const uint256_t & rhs){
    return rhs * lhs;
}

UINT256_T_EXTERN uint128_t & operator*=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator*=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (rhs * lhs);
}

UINT256_T_EXTERN uint256_t operator/(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
uint256_t operator/(const T & lhs, const uint256_t & rhs){
    return uint256_t(lhs) / rhs;
}

UINT256_T_EXTERN uint128_t & operator/=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator/=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (uint256_t(lhs) / rhs);
}

UINT256_T_EXTERN uint256_t operator%(const uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
uint256_t operator%(const T & lhs, const uint256_t & rhs){
    return uint256_t(lhs) % rhs;
}

UINT256_T_EXTERN uint128_t & operator%=(uint128_t & lhs, const uint256_t & rhs);

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
T & operator%=(T & lhs, const uint256_t & rhs){
    return lhs = static_cast <T> (uint256_t(lhs) % rhs);
}

// IO Operator
UINT256_T_EXTERN std::ostream & operator<<(std::ostream & stream, const uint256_t & rhs);
#endif
