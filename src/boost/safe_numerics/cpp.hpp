#ifndef BOOST_NUMERIC_CPP_HPP
#define BOOST_NUMERIC_CPP_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// policy which creates results types equal to that of C++ promotions.
// Using the policy will permit the program to build and run in release
// mode which is identical to that in debug mode except for the fact
// that errors aren't trapped. 

#include <type_traits> // integral constant, remove_cv, conditional
#include <limits>
#include <boost/integer.hpp> // integer type selection

#include "safe_common.hpp"
#include "checked_result.hpp"

namespace boost {
namespace safe_numerics {

// in C++ the following rules govern integer arithmetic

// This policy is use to emulate another compiler/machine architecture
// For example, a Z80 has 8 bit char, 16 bit short, 16 bit int, 32 bit long.  So one
// would use cpp<8, 16, 16, 32, 32> to test programs destined to run on a Z80

// Follow section 5 of the standard.
template<
    int CharBits,
    int ShortBits,
    int IntBits,
    int LongBits,
    int LongLongBits
>
struct cpp {
public:
    using local_char_type = typename boost::int_t<CharBits>::exact;
    using local_short_type = typename boost::int_t<ShortBits>::exact;
    using local_int_type = typename boost::int_t<IntBits>::exact;
    using local_long_type = typename boost::int_t<LongBits>::exact;
    using local_long_long_type = typename boost::int_t<LongLongBits>::exact;

    template<class T>
    using rank =
        typename std::conditional<
            std::is_same<local_char_type, typename std::make_signed<T>::type>::value,
            std::integral_constant<int, 1>,
        typename std::conditional<
            std::is_same<local_short_type, typename std::make_signed<T>::type>::value,
            std::integral_constant<int, 2>,
        typename std::conditional<
            std::is_same<local_int_type, typename std::make_signed<T>::type>::value,
            std::integral_constant<int, 3>,
        typename std::conditional<
            std::is_same<local_long_type, typename std::make_signed<T>::type>::value,
            std::integral_constant<int, 4>,
        typename std::conditional<
            std::is_same<local_long_long_type, typename std::make_signed<T>::type>::value,
            std::integral_constant<int, 5>,
            std::integral_constant<int, 6> // catch all - never promote integral
        >::type >::type >::type >::type >::type;

    // section 4.5 integral promotions

   // convert smaller of two types to the size of the larger
    template<class T, class U>
    using higher_ranked_type = typename std::conditional<
        (rank<T>::value < rank<U>::value),
        U,
        T
    >::type;

    template<class T, class U>
    using copy_sign = typename std::conditional<
        std::is_signed<U>::value,
        typename std::make_signed<T>::type,
        typename std::make_unsigned<T>::type
    >::type;

    template<class T>
    using integral_promotion = copy_sign<
        higher_ranked_type<local_int_type, T>,
        T
    >;

    // note presumption that T & U don't have he same sign
    // if that's not true, these won't work
    template<class T, class U>
    using select_signed = typename std::conditional<
        std::numeric_limits<T>::is_signed,
        T,
        U
    >::type;

    template<class T, class U>
    using select_unsigned = typename std::conditional<
        std::numeric_limits<T>::is_signed,
        U,
        T
    >::type;

    // section 5 clause 11 - usual arithmetic conversions
    template<typename T, typename U>
    using usual_arithmetic_conversions =
        // clause 0 - if both operands have the same type
        typename std::conditional<
            std::is_same<T, U>::value,
            // no further conversion is needed
            T,
        // clause 1 - otherwise if both operands have the same sign
        typename std::conditional<
            std::numeric_limits<T>::is_signed
            == std::numeric_limits<U>::is_signed,
            // convert to the higher ranked type
            higher_ranked_type<T, U>,
        // clause 2 - otherwise if the rank of he unsigned type exceeds
        // the rank of the of the signed type
        typename std::conditional<
            rank<select_unsigned<T, U>>::value
            >= rank< select_signed<T, U>>::value,
            // use unsigned type
            select_unsigned<T, U>,
        // clause 3 - otherwise if the type of the signed integer type can
        // represent all the values of the unsigned type
        typename std::conditional<
            std::numeric_limits< select_signed<T, U>>::digits >=
            std::numeric_limits< select_unsigned<T, U>>::digits,
            // use signed type
            select_signed<T, U>,
        // clause 4 - otherwise use unsigned version of the signed type
            std::make_signed< select_signed<T, U>>
        >::type >::type >::type
    >;

    template<typename T, typename U>
    using result_type = typename usual_arithmetic_conversions<
        integral_promotion<typename base_type<T>::type>,
        integral_promotion<typename base_type<U>::type>
    >::type;
public:
    template<typename T, typename U>
    struct addition_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct subtraction_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct multiplication_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct division_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct modulus_result {
       using type = result_type<T, U>;
    };
    // note: comparison_result (<, >, ...) is special.
    // The return value is always a bool.  The type returned here is
    // the intermediate type applied to make the values comparable.
    template<typename T, typename U>
    struct comparison_result {
        using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct left_shift_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct right_shift_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct bitwise_and_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct bitwise_or_result {
       using type = result_type<T, U>;
    };
    template<typename T, typename U>
    struct bitwise_xor_result {
       using type = result_type<T, U>;
    };
};

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_cpp_HPP
