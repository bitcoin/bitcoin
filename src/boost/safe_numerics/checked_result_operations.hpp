#ifndef BOOST_NUMERIC_CHECKED_RESULT_OPERATIONS
#define BOOST_NUMERIC_CHECKED_RESULT_OPERATIONS

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Implemenation of arithmetic on "extended" integers.
// Extended integers are defined in terms of C++ primitive integers as
//     a) an interger range
//     b) extra elements +inf, -inf, indeterminate
//
// Integer operations are closed on the set of extended integers
// but operations are not necessarily associative when they result in the
// extensions +inf, -inf, and indeterminate
//
// in this code, the type "checked_result<T>" where T is some
// integer type is an "extended" integer.

#include <cassert>

#include <boost/logic/tribool.hpp>

#include "checked_result.hpp"
#include "checked_integer.hpp"

//////////////////////////////////////////////////////////////////////////
// the following idea of "value_type" is used by several of the operations
// defined by checked_result arithmetic.

namespace boost {
namespace safe_numerics {

template<typename T>
constexpr inline void display(const boost::safe_numerics::checked_result<T> & c){
    switch(c.m_e){
    case safe_numerics_error::success:
        std::terminate();
    case safe_numerics_error::positive_overflow_error:    // result is above representational maximum
        std::terminate();
    case safe_numerics_error::negative_overflow_error:    // result is below representational minimum
        std::terminate();
    case safe_numerics_error::domain_error:               // one operand is out of valid range
        std::terminate();
    case safe_numerics_error::range_error:                // result cannot be produced for this operation
        std::terminate();
    case safe_numerics_error::precision_overflow_error:   // result lost precision
        std::terminate();
    case safe_numerics_error::underflow_error:            // result is too small to be represented
        std::terminate();
    case safe_numerics_error::negative_value_shift:       // negative value in shift operator
        std::terminate();
    case safe_numerics_error::negative_shift:             // shift a negative value
        std::terminate();
    case safe_numerics_error::shift_too_large:            // l/r shift exceeds variable size
        std::terminate();
    case safe_numerics_error::uninitialized_value:        // creating of uninitialized value
        std::terminate();
    }
}

//////////////////////////////////////////////////////////////////////////
// implement C++ operators for check_result<T>

struct sum_value_type {
    // characterization of various values
    const enum flag {
        known_value = 0,
        less_than_min,
        greater_than_max,
        indeterminate,
        count
    } m_flag;
    template<class T>
    constexpr flag to_flag(const checked_result<T> & t) const {
        switch(static_cast<safe_numerics_error>(t)){
        case safe_numerics_error::success:
            return known_value;
        case safe_numerics_error::negative_overflow_error:
            // result is below representational minimum
            return less_than_min;
        case safe_numerics_error::positive_overflow_error:
            // result is above representational maximum
            return greater_than_max;
        default:
            return indeterminate;
        }
    }
    template<class T>
    constexpr sum_value_type(const checked_result<T> & t) :
        m_flag(to_flag(t))
    {}
    constexpr operator std::uint8_t () const {
        return static_cast<std::uint8_t>(m_flag);
    }
};

// integers addition
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator+(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = sum_value_type;
    const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    // note major pain.  Clang constexpr multi-dimensional array is fine.
    // but gcc doesn't permit a multi-dimensional array to be be constexpr.
    // so we need to some ugly gymnastics to make our system work for all
    // all systems.
    const enum safe_numerics_error result[order * order] = {
        // t == known_value
        //{
            // u == ...
            safe_numerics_error::success,                   // known_value,
            safe_numerics_error::negative_overflow_error,   // less_than_min,
            safe_numerics_error::positive_overflow_error,   // greater_than_max,
            safe_numerics_error::range_error,               // indeterminate,
        //},
        // t == less_than_min,
        //{
            // u == ...
            safe_numerics_error::negative_overflow_error,   // known_value,
            safe_numerics_error::negative_overflow_error,   // less_than_min,
            safe_numerics_error::range_error,               // greater_than_max,
            safe_numerics_error::range_error,               // indeterminate,
        //},
        // t == greater_than_max,
        //{
            // u == ...
            safe_numerics_error::positive_overflow_error,   // known_value,
            safe_numerics_error::range_error,               // less_than_min,
            safe_numerics_error::positive_overflow_error,   // greater_than_max,
            safe_numerics_error::range_error,               // indeterminate,
        //},
        // t == indeterminate,
        //{
            // u == ...
            safe_numerics_error::range_error,      // known_value,
            safe_numerics_error::range_error,      // less_than_min,
            safe_numerics_error::range_error,      // greater_than_max,
            safe_numerics_error::range_error,      // indeterminate,
        //},
    };

    const value_type tx(t);
    const value_type ux(u);

    const safe_numerics_error e = result[tx * order + ux];
    if(safe_numerics_error::success == e)
        return checked::add<T>(t, u);
    return checked_result<T>(e, "addition result");
}

// unary +
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator+(
    const checked_result<T> & t
){
    return t;
}

// integers subtraction
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator-(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = sum_value_type;
    constexpr const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    constexpr const enum safe_numerics_error result[order * order] = {
        // t == known_value
        //{
            // u == ...
            safe_numerics_error::success,                   // known_value,
            safe_numerics_error::positive_overflow_error,   // less_than_min,
            safe_numerics_error::negative_overflow_error,   // greater_than_max,
            safe_numerics_error::range_error,               // indeterminate,
        //},
        // t == less_than_min,
        //{
            // u == ...
            safe_numerics_error::negative_overflow_error,   // known_value,
            safe_numerics_error::range_error,               // less_than_min,
            safe_numerics_error::negative_overflow_error,   // greater_than_max,
            safe_numerics_error::range_error,               // indeterminate,
        //},
        // t == greater_than_max,
        //{
            // u == ...
            safe_numerics_error::positive_overflow_error,   // known_value,
            safe_numerics_error::positive_overflow_error,   // less_than_min,
            safe_numerics_error::range_error,               // greater_than_max,
            safe_numerics_error::range_error,               // indeterminate,
        //},
        // t == indeterminate,
        //{
            // u == ...
            safe_numerics_error::range_error,               // known_value,
            safe_numerics_error::range_error,               // less_than_min,
            safe_numerics_error::range_error,               // greater_than_max,
            safe_numerics_error::range_error,               // indeterminate,
        //},
    };

    const value_type tx(t);
    const value_type ux(u);

    const safe_numerics_error e = result[tx * order + ux];
    if(safe_numerics_error::success == e)
        return checked::subtract<T>(t, u);
    return checked_result<T>(e, "subtraction result");
}

// unary -
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator-(
    const checked_result<T> & t
){
//    assert(false);
    return checked_result<T>(0) - t;
}

struct product_value_type {
    // characterization of various values
    const enum flag {
        less_than_min = 0,
        less_than_zero,
        zero,
        greater_than_zero,
        greater_than_max,
        indeterminate,
        // count of number of cases for values
        count,
        // temporary values for special cases
        t_value,
        u_value,
        z_value
    } m_flag;
    template<class T>
    constexpr flag to_flag(const checked_result<T> & t) const {
        switch(static_cast<safe_numerics_error>(t)){
        case safe_numerics_error::success:
            return (t < checked_result<T>(0))
                ? less_than_zero
                : (t > checked_result<T>(0))
                ? greater_than_zero
                : zero;
        case safe_numerics_error::negative_overflow_error:
            // result is below representational minimum
            return less_than_min;
        case safe_numerics_error::positive_overflow_error:
            // result is above representational maximum
            return greater_than_max;
        default:
            return indeterminate;
        }
    }
    template<class T>
    constexpr product_value_type(const checked_result<T> & t) :
        m_flag(to_flag(t))
    {}
    constexpr operator std::uint8_t () const {
        return static_cast<std::uint8_t>(m_flag);
    }
};

// integers multiplication
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator*(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = product_value_type;
    const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    constexpr const enum value_type::flag result[order * order] = {
        // t == less_than_min
        //{
            // u == ...
            value_type::greater_than_max,   // less_than_min,
            value_type::greater_than_max,   // less_than_zero,
            value_type::zero,               // zero,
            value_type::less_than_min,      // greater_than_zero,
            value_type::less_than_min,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == less_than_zero,
        //{
            // u == ...
            value_type::greater_than_max,   // less_than_min,
            value_type::greater_than_zero,  // less_than_zero,
            value_type::zero,               // zero,
            value_type::less_than_zero,     // greater_than_zero,
            value_type::less_than_min,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == zero,
        //{
            // u == ...
            value_type::zero,               // less_than_min,
            value_type::zero,               // less_than_zero,
            value_type::zero,               // zero,
            value_type::zero,               // greater_than_zero,
            value_type::zero,               // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == greater_than_zero,
        //{
            // u == ...
            value_type::less_than_min,      // less_than_min,
            value_type::less_than_zero,     // less_than_zero,
            value_type::zero,               // zero,
            value_type::greater_than_zero,  // greater_than_zero,
            value_type::greater_than_max,   // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == greater_than_max
        //{
            value_type::less_than_min,      // less_than_min,
            value_type::less_than_min,      // less_than_zero,
            value_type::zero,               // zero,
            value_type::greater_than_max,   // greater_than_zero,
            value_type::greater_than_max,   // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == indeterminate
        //{
            value_type::indeterminate,      // less_than_min,
            value_type::indeterminate,      // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::indeterminate,      // greater_than_zero,
            value_type::indeterminate,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //}
    };

    const value_type tx(t);
    const value_type ux(u);

    switch(result[tx * order + ux]){
        case value_type::less_than_min:
            return safe_numerics_error::negative_overflow_error;
        case value_type::zero:
            return T(0);
        case value_type::greater_than_max:
            return safe_numerics_error::positive_overflow_error;
        case value_type::less_than_zero:
        case value_type::greater_than_zero:
            return checked::multiply<T>(t, u);
        case value_type::indeterminate:
            return safe_numerics_error::range_error;
        default:
            assert(false);
        }
    return checked_result<T>(0); // to suppress msvc warning
}

// integers division
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator/(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = product_value_type;
    const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    constexpr const enum value_type::flag result[order * order] = {
        // t == less_than_min
        //{
            // u == ...
            value_type::indeterminate,   // less_than_min,
            value_type::greater_than_max,   // less_than_zero,
            value_type::less_than_min,      // zero,
            value_type::less_than_min,      // greater_than_zero,
            value_type::less_than_min,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == less_than_zero,
        //{
            // u == ...
            value_type::zero,               // less_than_min,
            value_type::greater_than_zero,  // less_than_zero,
            value_type::less_than_min,      // zero,
            value_type::less_than_zero,     // greater_than_zero,
            value_type::zero,               // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == zero,
        //{
            // u == ...
            value_type::zero,               // less_than_min,
            value_type::zero,               // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::zero,               // greater_than_zero,
            value_type::zero,               // greater than max,
            value_type::indeterminate,               // indeterminate,
        //},
        // t == greater_than_zero,
        //{
            // u == ...
            value_type::zero,               // less_than_min,
            value_type::less_than_zero,     // less_than_zero,
            value_type::greater_than_max,   // zero,
            value_type::greater_than_zero,  // greater_than_zero,
            value_type::zero,               // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == greater_than_max
        //{
            value_type::less_than_min,      // less_than_min,
            value_type::less_than_min,      // less_than_zero,
            value_type::greater_than_max,   // zero,
            value_type::greater_than_max,   // greater_than_zero,
            value_type::indeterminate,   // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == indeterminate
        //{
            value_type::indeterminate,      // less_than_min,
            value_type::indeterminate,      // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::indeterminate,      // greater_than_zero,
            value_type::indeterminate,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //}
    };

    const value_type tx(t);
    const value_type ux(u);

    switch(result[tx * order + ux]){
        case value_type::less_than_min:
            return safe_numerics_error::negative_overflow_error;
        case value_type::zero:
            return 0;
        case value_type::greater_than_max:
            return safe_numerics_error::positive_overflow_error;
        case value_type::less_than_zero:
        case value_type::greater_than_zero:
            return checked::divide<T>(t, u);
        case value_type::indeterminate:
            return safe_numerics_error::range_error;
        default:
            assert(false);
    }
    return checked_result<T>(0); // to suppress msvc warning
}

// integers modulus
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator%(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = product_value_type;
    const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    constexpr const enum value_type::flag result[order * order] = {
        // t == less_than_min
        //{
            // u == ...
            value_type::indeterminate,      // less_than_min,
            value_type::z_value,            // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::z_value,            // greater_than_zero,
            value_type::indeterminate,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == less_than_zero,
        //{
            // u == ...
            value_type::t_value,            // less_than_min,
            value_type::greater_than_zero,  // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::less_than_zero,     // greater_than_zero,
            value_type::t_value,            // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == zero,
        //{
            // u == ...
            value_type::zero,               // less_than_min,
            value_type::zero,               // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::zero,               // greater_than_zero,
            value_type::zero,               // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == greater_than_zero,
        //{
            // u == ...
            value_type::t_value,            // less_than_min,
            value_type::less_than_zero,     // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::greater_than_zero,  // greater_than_zero,
            value_type::t_value,            // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == greater_than_max
        //{
            value_type::indeterminate,      // less_than_min,
            value_type::u_value,            // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::u_value,            // greater_than_zero,
            value_type::indeterminate,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //},
        // t == indeterminate
        //{
            value_type::indeterminate,      // less_than_min,
            value_type::indeterminate,      // less_than_zero,
            value_type::indeterminate,      // zero,
            value_type::indeterminate,      // greater_than_zero,
            value_type::indeterminate,      // greater than max,
            value_type::indeterminate,      // indeterminate,
        //}
    };

    const value_type tx(t);
    const value_type ux(u);

    switch(result[tx * order + ux]){
        case value_type::zero:
            return 0;
        case value_type::less_than_zero:
        case value_type::greater_than_zero:
            return checked::modulus<T>(t, u);
        case value_type::indeterminate:
            return safe_numerics_error::range_error;
        case value_type::t_value:
            return t;
        case value_type::u_value:
            return checked::subtract<T>(u, 1);
        case value_type::z_value:
            return checked::subtract<T>(1, u);
        case value_type::greater_than_max:
        case value_type::less_than_min:
        default:
            assert(false);
    }
    // suppress msvc warning
    return checked_result<T>(0);
}

// comparison operators

template<class T>
constexpr boost::logic::tribool operator<(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = sum_value_type;
    constexpr const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    // the question arises about how to order values of type greater_than_min.
    // that is: what should greater_than_min < greater_than_min return.
    //
    // a) return indeterminate because we're talking about the "true" values for
    //    which greater_than_min is a placholder.
    //
    // b) return false because the two values are "equal"
    //
    // for our purposes, a) seems the better interpretation.
    
    enum class result_type : std::uint8_t {
        runtime,
        false_value,
        true_value,
        indeterminate,
    };
    constexpr const result_type resultx[order * order]{
        // t == known_value
        //{
            // u == ...
            result_type::runtime,       // known_value,
            result_type::false_value,   // less_than_min,
            result_type::true_value,    // greater_than_max,
            result_type::indeterminate, // indeterminate,
        //},
        // t == less_than_min
        //{
            // u == ...
            result_type::true_value,    // known_value,
            result_type::indeterminate, // less_than_min, see above argument
            result_type::true_value,    // greater_than_max,
            result_type::indeterminate, // indeterminate,
        //},
        // t == greater_than_max
        //{
            // u == ...
            result_type::false_value,   // known_value,
            result_type::false_value,   // less_than_min,
            result_type::indeterminate, // greater_than_max, see above argument
            result_type::indeterminate, // indeterminate,
        //},
        // t == indeterminate
        //{
            // u == ...
            result_type::indeterminate, // known_value,
            result_type::indeterminate, // less_than_min,
            result_type::indeterminate, // greater_than_max,
            result_type::indeterminate, // indeterminate,
        //},
    };

    const value_type tx(t);
    const value_type ux(u);

    switch(resultx[tx * order + ux]){
    case result_type::runtime:
        return static_cast<const T &>(t) < static_cast<const T &>(u);
    case result_type::false_value:
        return false;
    case result_type::true_value:
        return true;
    case result_type::indeterminate:
        return boost::logic::indeterminate;
    default:
        assert(false);
    }
    return true;
}

template<class T>
constexpr boost::logic::tribool
operator>=(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    return !(t < u);
}

template<class T>
constexpr boost::logic::tribool
operator>(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    return u < t;
}

template<class T>
constexpr boost::logic::tribool
operator<=(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    return !(u < t);
}

template<class T>
constexpr boost::logic::tribool
operator==(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = sum_value_type;
    constexpr const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    enum class result_type : std::uint8_t {
        runtime,
        false_value,
        true_value,
        indeterminate,
    };

    constexpr const result_type result[order * order]{
        // t == known_value
        //{
            // u == ...
            result_type::runtime,       // known_value,
            result_type::false_value,   // less_than_min,
            result_type::false_value,   // greater_than_max,
            result_type::indeterminate, // indeterminate,
        //},
        // t == less_than_min
        //{
            // u == ...
            result_type::false_value,   // known_value,
            result_type::indeterminate, // less_than_min,
            result_type::false_value,   // greater_than_max,
            result_type::indeterminate, // indeterminate,
        //},
        // t == greater_than_max
        //{
            // u == ...
            result_type::false_value,   // known_value,
            result_type::false_value,   // less_than_min,
            result_type::indeterminate, // greater_than_max,
            result_type::indeterminate, // indeterminate,
        //},
        // t == indeterminate
        //{
            // u == ...
            result_type::indeterminate, // known_value,
            result_type::indeterminate, // less_than_min,
            result_type::indeterminate, // greater_than_max,
            result_type::indeterminate, // indeterminate,
        //},
    };

    const value_type tx(t);
    const value_type ux(u);

    switch(result[tx * order + ux]){
    case result_type::runtime:
        return static_cast<const T &>(t) == static_cast<const T &>(u);
    case result_type::false_value:
        return false;
    case result_type::true_value:
        return true;
    case result_type::indeterminate:
        return boost::logic::indeterminate;
    default:
        assert(false);
    }
    // suppress msvc warning - not all control paths return a value
    return false;
}

template<class T>
constexpr boost::logic::tribool
operator!=(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    return ! (t == u);
}

template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator>>(
    const checked_result<T> & t,
    const checked_result<T> & u
);

template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator~(
    const checked_result<T> & t
){
//    assert(false);
    return ~t.m_r;
}

template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator<<(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = product_value_type;
    const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    constexpr const std::uint8_t result[order * order] = {
        // t == less_than_min
        //{
            // u == ...
            1, // -1,                                           // less_than_min,
            2, // safe_numerics_error::negative_overflow_error, // less_than_zero,
            2, // safe_numerics_error::negative_overflow_error, // zero,
            2, // safe_numerics_error::negative_overflow_error, // greater_than_zero,
            2, // safe_numerics_error::negative_overflow_error, // greater than max,
            1, // safe_numerics_error::range_error,             // indeterminate,
        //},
        // t == less_than_zero,
        //{
            // u == ...
            3, // -1,                                           // less_than_min,
            4, // - (-t >> -u),                                 // less_than_zero,
            5, // safe_numerics_error::negative_overflow_error, // zero,
            6, // - (-t << u),                                  // greater_than_zero,
            2, // safe_numerics_error::negative_overflow_error, // greater than max,
            1, // safe_numerics_error::range_error,             // indeterminate,
        //},
        // t == zero,
        //{
            // u == ...
            3, // 0     // less_than_min,
            3, // 0     // less_than_zero,
            3, // 0,    // zero,
            3, // 0,    // greater_than_zero,
            3, // 0,    // greater than max,
            3, // safe_numerics_error::range_error,    // indeterminate,
        //},
        // t == greater_than_zero,
        //{
            // u == ...
            3, // 0,                                            // less_than_min,
            7, // t << -u,                                      // less_than_zero,
            5, // t,                                            // zero,
            8, // t << u                                        // greater_than_zero,
            9, // safe_numerics_error::positive_overflow_error, // greater than max,
            1, // safe_numerics_error::range_error,             // indeterminate,
        //},
        // t == greater_than_max
        //{
            // u == ...
            1, // safe_numerics_error::range_error,               // less_than_min,
            9, // safe_numerics_error::positive_overflow_error),  // less_than_zero,
            9, // safe_numerics_error::positive_overflow_error,   // zero,
            9, // safe_numerics_error::positive_overflow_error),  // greater_than_zero,
            9, // safe_numerics_error::positive_overflow_error,   // greater than max,
            1, // safe_numerics_error::range_error,               // indeterminate,
        //},
        // t == indeterminate
        //{
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
        //}
    };

    const value_type tx(t);
    const value_type ux(u);
    assert(tx * order + ux < order * order);

    // I had a switch(i) statment here - but it results in an ICE
    // on multiple versions of gcc.  So make the equivalent in
    // nested if statments - should be the same (more or less)
    // performancewise.
    const unsigned int i = result[tx * order + ux];
    assert(i <= 9);
    if(1 == i){
        return safe_numerics_error::range_error;
    }
    else
    if(2 == i){
        return safe_numerics_error::negative_overflow_error;
    }
    else
    if(3 == i){
        return checked_result<T>(0);
    // the following gymnastics are to handle the case where 
    // a value is changed from a negative to a positive number.
    // For example, and 8 bit number t == -128.  Then -t also
    // equals -128 since 128 cannot be held in an 8 bit signed
    // integer.
    }
    else
    if(4 == i){ // - (-t >> -u)
        assert(static_cast<bool>(t < checked_result<T>(0)));
        assert(static_cast<bool>(u < checked_result<T>(0)));
        return t >> -u;
    }
    else
    if(5 == i){
        return t;
    }
    else
    if(6 == i){ // - (-t << u)
        assert(static_cast<bool>(t < checked_result<T>(0)));
        assert(static_cast<bool>(u > checked_result<T>(0)));
        const checked_result<T> temp_t = t * checked_result<T>(2);
        const checked_result<T> temp_u = u - checked_result<T>(1);
        return  - (-temp_t << temp_u);
    }
    else
    if(7 == i){  // t >> -u
        assert(static_cast<bool>(t > checked_result<T>(0)));
        assert(static_cast<bool>(u < checked_result<T>(0)));
        return t >> -u;
    }
    else
    if(8 == i){ // t << u
        assert(static_cast<bool>(t > checked_result<T>(0)));
        assert(static_cast<bool>(u > checked_result<T>(0)));
        checked_result<T> r = checked::left_shift<T>(t, u);
        return (r.m_e == safe_numerics_error::shift_too_large)
        ? checked_result<T>(safe_numerics_error::positive_overflow_error)
        : r;
    }
    else
    if(9 == i){
        return safe_numerics_error::positive_overflow_error;
    }
    else{
        assert(false);
    };
    return checked_result<T>(0); // to suppress msvc warning
}

template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator>>(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    using value_type = product_value_type;
    const std::uint8_t order = static_cast<std::uint8_t>(value_type::count);

    const std::uint8_t result[order * order] = {
        // t == less_than_min
        //{
            // u == ...
            2, // safe_numerics_error::negative_overflow_error, // less_than_min,
            2, // safe_numerics_error::negative_overflow_error, // less_than_zero,
            2, // safe_numerics_error::negative_overflow_error, // zero,
            2, // safe_numerics_error::negative_overflow_error, // greater_than_zero,
            1, // safe_numerics_error::range_error,             // greater than max,
            1, // safe_numerics_error::range_error,             // indeterminate,
        //},
        // t == less_than_zero,
        //{
            // u == ...
            2, // safe_numerics_error::negative_overflow_error  // less_than_min,
            4, // - (-t << -u),                                 // less_than_zero,
            5, // safe_numerics_error::negative_overflow_error. // zero,
            6, // - (-t >> u),                                  // greater_than_zero,
            3, // 0, ? or -1                                    // greater than max,
            1, // safe_numerics_error::range_error,             // indeterminate,
        //},
        // t == zero,
        //{
            // u == ...
            3, // 0     // less_than_min,
            3, // 0     // less_than_zero,
            3, // 0,    // zero,
            3, // 0,    // greater_than_zero,
            3, // 0,    // greater than max,
            3, // safe_numerics_error::range_error,    // indeterminate,
        //},
        // t == greater_than_zero,
        //{
            // u == ...
            9, // safe_numerics_error::positive_overflow_error  // less_than_min,
            7, // t << -u,                                      // less_than_zero,
            5, // t,                                            // zero,
            8, // t >> u                                        // greater_than_zero,
            3, // 0,                                            // greater than max,
            1, // safe_numerics_error::range_error,             // indeterminate,
        //},
        // t == greater_than_max
        //{
            // u == ...
            9, // safe_numerics_error::positive_overflow_error, // less_than_min,
            9, // safe_numerics_error::positive_overflow_error, // less_than_zero,
            9, // safe_numerics_error::positive_overflow_error, // zero,
            9, // safe_numerics_error::positive_overflow_error, // greater_than_zero,
            1, // safe_numerics_error::range_error,             // greater than max,
            1, // safe_numerics_error::range_error,             // indeterminate,
        //},
        // t == indeterminate
        //{
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
            1, // safe_numerics_error::range_error,    // indeterminate,
        //}
    };

    const value_type tx(t);
    const value_type ux(u);
    assert(tx * order + ux < order * order);

    // I had a switch(i) statment here - but it results in an ICE
    // on multiple versions of gcc.  So make the equivalent in
    // nested if statments - should be the same (more or less)
    // performancewise.
    const unsigned int i = result[tx * order + ux];
    assert(i <= 9);
    if(1 == i){
        return safe_numerics_error::range_error;
    }
    else
    if(2 == i){
        return safe_numerics_error::negative_overflow_error;
    }
    else
    if(3 == i){
        return checked_result<T>(0);
    }
    else
    if(4 == i){ // - (-t << -u)
        assert(static_cast<bool>(t < checked_result<T>(0)));
        assert(static_cast<bool>(u < checked_result<T>(0)));
        return t << -u;
    }
    else
    if(5 == i){
        return t;
    }
    else
    if(6 == i){ //  - (-t >> u)
        assert(static_cast<bool>(t < checked_result<T>(0)));
        assert(static_cast<bool>(u > checked_result<T>(0)));
        const checked_result<T> temp_t = t / checked_result<T>(2);
        const checked_result<T> temp_u = u - checked_result<T>(1);
        return  - (-temp_t >> temp_u);
    }
    else
    if(7 == i){  // t << -u,
        assert(static_cast<bool>(t > checked_result<T>(0)));
        assert(static_cast<bool>(u < checked_result<T>(0)));
        return t << -u;
    }
    else
    if(8 == i){ // t >> u
        assert(static_cast<bool>(t > checked_result<T>(0)));
        assert(static_cast<bool>(u > checked_result<T>(0)));
        checked_result<T> r = checked::right_shift<T>(t, u);
        return (r.m_e == safe_numerics_error::shift_too_large)
        ? checked_result<T>(0)
        : r;
    }
    else
    if(9 == i){
        return safe_numerics_error::positive_overflow_error;
    }
    else{
        assert(false);
    };
    return checked_result<T>(0); // to suppress msvc warning
}

template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator|(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    return
        t.exception() || u.exception()
        ? checked_result<T>(safe_numerics_error::range_error)
        : checked::bitwise_or<T>(
            static_cast<T>(t),
            static_cast<T>(u)
        );
}
template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator^(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    return
        t.exception() || u.exception()
        ? checked_result<T>(safe_numerics_error::range_error)
        : checked::bitwise_xor<T>(
            static_cast<T>(t),
            static_cast<T>(u)
        );
}

template<class T>
typename std::enable_if<
    std::is_integral<T>::value,
    checked_result<T>
>::type
constexpr inline operator&(
    const checked_result<T> & t,
    const checked_result<T> & u
){
    return
        t.exception() || u.exception()
        ? checked_result<T>(safe_numerics_error::range_error)
        : checked::bitwise_and<T>(
            static_cast<T>(t),
            static_cast<T>(u)
        );
}

} // safe_numerics
} // boost

#include <iosfwd>

namespace std {

template<typename CharT, typename Traits, typename R>
inline std::basic_ostream<CharT, Traits> & operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const boost::safe_numerics::checked_result<R> & r
){
    bool e = r.exception();
    os << e;
    if(!e)
        os << static_cast<R>(r);
    else
        os << std::error_code(r.m_e).message() << ':' << static_cast<char const *>(r);
    return os;
}

template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> & operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const boost::safe_numerics::checked_result<signed char> & r
){
    bool e = r.exception();
    os << e;
    if(! e)
        os << static_cast<std::int16_t>(r);
    else
        os << std::error_code(r.m_e).message() << ':' << static_cast<char const *>(r);
    return os;
}

template<typename CharT, typename Traits, typename R>
inline std::basic_istream<CharT, Traits> & operator>>(
    std::basic_istream<CharT, Traits> & is,
    boost::safe_numerics::checked_result<R> & r
){
    bool e;
    is >> e;
    if(!e)
        is >> static_cast<R>(r);
    else
        is >> std::error_code(r.m_e).message() >> ':' >> static_cast<char const *>(r);
    return is;
}

template<typename CharT, typename Traits>
inline std::basic_istream<CharT, Traits> & operator>>(
    std::basic_istream<CharT, Traits> & is, 
    boost::safe_numerics::checked_result<signed char> & r
){
    bool e;
    is >> e;
    if(!e){
        std::int16_t i;
        is >> i;
        r.m_contents.m_r = static_cast<signed char>(i);
    }
    else
        is >> std::error_code(r.m_e).message() >> ':' >> static_cast<char const *>(r);
    return is;
}

} // std

/////////////////////////////////////////////////////////////////
// numeric limits for checked<R>

#include <limits>

namespace std {

template<class R>
class numeric_limits<boost::safe_numerics::checked_result<R> >
    : public std::numeric_limits<R>
{
    using this_type = boost::safe_numerics::checked_result<R>;
public:
    constexpr static this_type min() noexcept {
        return this_type(std::numeric_limits<R>::min());
    }
    constexpr static this_type max() noexcept {
        return this_type(std::numeric_limits<R>::max());
    }
};

} // std

#endif  // BOOST_NUMERIC_CHECKED_RESULT_OPERATIONS
