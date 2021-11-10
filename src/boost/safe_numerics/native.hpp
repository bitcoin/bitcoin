#ifndef BOOST_SAFE_NUMERICS_NATIVE_HPP
#define BOOST_SAFE_NUMERICS_NATIVE_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <type_traits>
#include <limits>

// policy which creates results types and values equal to that of C++ promotions.
// When used in conjunction with a desired exception policy, traps errors but
// does not otherwise alter the results produced by the program using it.
namespace boost {
namespace safe_numerics {

struct native {
public:
    // arithmetic operators
    template<typename T, typename U>
    struct addition_result {
        using type = decltype(
            typename base_type<T>::type()
            + typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct subtraction_result {
        using type = decltype(
            typename base_type<T>::type()
            - typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct multiplication_result {
        using type = decltype(
            typename base_type<T>::type()
            * typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct division_result {
        using type = decltype(
            typename base_type<T>::type()
            / typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct modulus_result {
        using type = decltype(
            typename base_type<T>::type()
            % typename base_type<U>::type()
        );
    };
    // note: comparison_result (<, >, ...) is special.
    // The return value is always a bool.  The type returned here is
    // the intermediate type applied to make the values comparable.
    template<typename T, typename U>
    struct comparison_result {
        using type = decltype(
            typename base_type<T>::type()
            + typename base_type<U>::type()
        );
    };

    // shift operators
    template<typename T, typename U>
    struct left_shift_result {
        using type = decltype(
            typename base_type<T>::type()
            << typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct right_shift_result {
        using type = decltype(
            typename base_type<T>::type()
            >> typename base_type<U>::type()
        );
    };
    // bitwise operators
    template<typename T, typename U>
    struct bitwise_or_result {
        using type = decltype(
            typename base_type<T>::type()
            | typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct bitwise_and_result {
        using type = decltype(
            typename base_type<T>::type()
            & typename base_type<U>::type()
        );
    };
    template<typename T, typename U>
    struct bitwise_xor_result {
        using type = decltype(
            typename base_type<T>::type()
            ^ typename base_type<U>::type()
        );
    };
};

} // safe_numerics
} // boost

#endif // BOOST_SAFE_NUMERICS_NATIVE_HPP
