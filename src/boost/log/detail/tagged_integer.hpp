/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   tagged_integer.hpp
 * \author Andrey Semashev
 * \date   11.01.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_TAGGED_INTEGER_HPP_INCLUDED_
#define BOOST_LOG_TAGGED_INTEGER_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A tagged integer wrapper for type safety
template< typename IntT, typename TagT >
struct tagged_integer
{
    //! Contained value type
    typedef IntT integer_type;
    //! Tag
    typedef TagT tag;

    //! Contained value
    integer_type value;

    //! Conversion operator
    BOOST_CONSTEXPR operator integer_type() const BOOST_NOEXCEPT { return value; }

    //  Increment
    tagged_integer& operator++ () BOOST_NOEXCEPT { ++value; return *this; }
    tagged_integer operator++ (int) BOOST_NOEXCEPT { tagged_integer temp = *this; ++value; return temp; }
    //  Decrement
    tagged_integer& operator-- () BOOST_NOEXCEPT { --value; return *this; }
    tagged_integer operator-- (int) BOOST_NOEXCEPT { tagged_integer temp = *this; --value; return temp; }

#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
    tagged_integer& operator op (tagged_integer const& that) BOOST_NOEXCEPT { value op that.value; return *this; }

    BOOST_LOG_TAGGED_INTEGER_OP(|=)
    BOOST_LOG_TAGGED_INTEGER_OP(&=)
    BOOST_LOG_TAGGED_INTEGER_OP(^=)
    BOOST_LOG_TAGGED_INTEGER_OP(+=)
    BOOST_LOG_TAGGED_INTEGER_OP(-=)
    BOOST_LOG_TAGGED_INTEGER_OP(*=)
    BOOST_LOG_TAGGED_INTEGER_OP(/=)
    BOOST_LOG_TAGGED_INTEGER_OP(%=)

#undef BOOST_LOG_TAGGED_INTEGER_OP

    //! Inversion operator
    tagged_integer& operator~ () BOOST_NOEXCEPT { ~value; return *this; }

    //  Shift operators
    template< typename T >
    tagged_integer& operator<<= (T const& that) BOOST_NOEXCEPT { value <<= that; return *this; }
    template< typename T >
    tagged_integer& operator>>= (T const& that) BOOST_NOEXCEPT { value >>= that; return *this; }

private:
    //  Protection against improper usage
    template< typename T1, typename T2 >
    tagged_integer& operator<<= (tagged_integer< T1, T2 > const&);
    template< typename T1, typename T2 >
    tagged_integer& operator>>= (tagged_integer< T1, T2 > const&);
};

    //  Relational operators
#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
    template< typename IntT, typename TagT >\
    inline bool operator op (\
        tagged_integer< IntT, TagT > const& left, tagged_integer< IntT, TagT > const& right) BOOST_NOEXCEPT\
    {\
        return (left.value op right.value);\
    }

BOOST_LOG_TAGGED_INTEGER_OP(==)
BOOST_LOG_TAGGED_INTEGER_OP(!=)
BOOST_LOG_TAGGED_INTEGER_OP(<)
BOOST_LOG_TAGGED_INTEGER_OP(>)
BOOST_LOG_TAGGED_INTEGER_OP(<=)
BOOST_LOG_TAGGED_INTEGER_OP(>=)

#undef BOOST_LOG_TAGGED_INTEGER_OP

#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
    template< typename IntT, typename TagT >\
    inline tagged_integer< IntT, TagT > operator op (\
        tagged_integer< IntT, TagT > const& left, tagged_integer< IntT, TagT > const& right) BOOST_NOEXCEPT\
    {\
        tagged_integer< IntT, TagT > temp = left;\
        temp op##= right;\
        return temp;\
    }

BOOST_LOG_TAGGED_INTEGER_OP(|)
BOOST_LOG_TAGGED_INTEGER_OP(&)
BOOST_LOG_TAGGED_INTEGER_OP(^)
BOOST_LOG_TAGGED_INTEGER_OP(+)
BOOST_LOG_TAGGED_INTEGER_OP(-)
BOOST_LOG_TAGGED_INTEGER_OP(*)
BOOST_LOG_TAGGED_INTEGER_OP(/)
BOOST_LOG_TAGGED_INTEGER_OP(%)

#undef BOOST_LOG_TAGGED_INTEGER_OP

#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
    template< typename IntT, typename TagT, typename T >\
    inline tagged_integer< IntT, TagT > operator op (\
        tagged_integer< IntT, TagT > const& left, T const& right) BOOST_NOEXCEPT\
    {\
        tagged_integer< IntT, TagT > temp = left;\
        temp op##= right;\
        return temp;\
    }

BOOST_LOG_TAGGED_INTEGER_OP(<<)
BOOST_LOG_TAGGED_INTEGER_OP(>>)

#undef BOOST_LOG_TAGGED_INTEGER_OP

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_TAGGED_INTEGER_HPP_INCLUDED_
