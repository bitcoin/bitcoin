#ifndef BOOST_NUMERIC_CHECKED_INTEGER_HPP
#define BOOST_NUMERIC_CHECKED_INTEGER_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// contains operations for doing checked aritmetic on NATIVE
// C++ types.

#include <limits>
#include <type_traits> // is_integral, make_unsigned, enable_if
#include <algorithm>   // std::max

#include "checked_result.hpp"
#include "checked_default.hpp"
#include "safe_compare.hpp"
#include "utility.hpp"
#include "exception.hpp"

namespace boost {
namespace safe_numerics {

// utility

template<bool tf>
using bool_type = typename std::conditional<tf, std::true_type, std::false_type>::type;

////////////////////////////////////////////////////
// layer 0 - implement safe operations for intrinsic integers
// Note presumption of twos complement integer arithmetic

// convert an integral value to some other integral type
template<
    typename R,
    R Min,
    R Max,
    typename T,
    class F
>
struct heterogeneous_checked_operation<
    R,
    Min,
    Max,
    T,
    F,
    typename std::enable_if<
        std::is_integral<R>::value
        && std::is_integral<T>::value
    >::type
>{
    ////////////////////////////////////////////////////
    // safe casting on primitive types

    struct cast_impl_detail {
        constexpr static checked_result<R>
        cast_impl(
            const T & t,
            std::true_type, // R is signed
            std::true_type  // T is signed
        ){
            // INT32-C Ensure that operations on signed
            // integers do not overflow
            return
            boost::safe_numerics::safe_compare::greater_than(t, Max) ?
                F::template invoke<safe_numerics_error::positive_overflow_error>(
                    "converted signed value too large"
                )
            : boost::safe_numerics::safe_compare::less_than(t, Min) ?
                F::template invoke<safe_numerics_error::negative_overflow_error>(
                    "converted signed value too small"
                )
            :
                checked_result<R>(static_cast<R>(t))
            ;
        }
        constexpr static checked_result<R>
        cast_impl(
            const T & t,
            std::true_type,  // R is signed
            std::false_type  // T is unsigned
        ){
            // INT30-C Ensure that unsigned integer operations
            // do not wrap
            return
            boost::safe_numerics::safe_compare::greater_than(t, Max) ?
                F::template invoke<safe_numerics_error::positive_overflow_error>(
                    "converted unsigned value too large"
                )
            :
            boost::safe_numerics::safe_compare::less_than(t, Min) ?
                F::template invoke<safe_numerics_error::positive_overflow_error>(
                    "converted unsigned value too small"
                )
            :
                checked_result<R>(static_cast<R>(t))
            ;
        }
        constexpr static checked_result<R>
        cast_impl(
            const T & t,
            std::false_type, // R is unsigned
            std::false_type  // T is unsigned
        ){
            // INT32-C Ensure that operations on unsigned
            // integers do not overflow
            return
            boost::safe_numerics::safe_compare::greater_than(t, Max) ?
                F::template invoke<safe_numerics_error::positive_overflow_error>(
                    "converted unsigned value too large"
                )
            :
            boost::safe_numerics::safe_compare::less_than(t, Min) ?
                F::template invoke<safe_numerics_error::positive_overflow_error>(
                    "converted unsigned value too small"
                )
            :
                checked_result<R>(static_cast<R>(t))
            ;
        }
        constexpr static checked_result<R>
        cast_impl(
            const T & t,
            std::false_type, // R is unsigned
            std::true_type   // T is signed
        ){
            return
            boost::safe_numerics::safe_compare::less_than(t, Min) ?
                F::template invoke<safe_numerics_error::domain_error>(
                    "converted value to low or negative"
                )
            :
            boost::safe_numerics::safe_compare::greater_than(t, Max) ?
                F::template invoke<safe_numerics_error::positive_overflow_error>(
                    "converted signed value too large"
                )
            :
                checked_result<R>(static_cast<R>(t))
            ;
        }
    }; // cast_impl_detail

    constexpr static checked_result<R>
    cast(const T & t){
        return
            cast_impl_detail::cast_impl(
                t,
                std::is_signed<R>(),
                std::is_signed<T>()
            );
    }
}; // heterogeneous_checked_operation

// converting floating point value to integral type
template<
    typename R,
    R Min,
    R Max,
    typename T,
    class F
>
struct heterogeneous_checked_operation<
    R,
    Min,
    Max,
    T,
    F,
    typename std::enable_if<
        std::is_integral<R>::value
        && std::is_floating_point<T>::value
    >::type
>{
    constexpr static checked_result<R>
    cast(const T & t){
        return static_cast<R>(t);
    }
}; // heterogeneous_checked_operation

// converting integral value to floating point type

// INT35-C. Use correct integer precisions
template<
    typename R,
    R Min,
    R Max,
    typename T,
    class F
>
struct heterogeneous_checked_operation<
    R,
    Min,
    Max,
    T,
    F,
    typename std::enable_if<
        std::is_floating_point<R>::value
        && std::is_integral<T>::value
     >::type
 >{
     constexpr static checked_result<R>
     cast(const T & t){
        if(std::numeric_limits<R>::digits < std::numeric_limits<T>::digits){
            if(utility::significant_bits(t) > std::numeric_limits<R>::digits){
                return F::invoke(
                    safe_numerics_error::precision_overflow_error,
                    "keep precision"
                );
            }
        }
        return t;
    }
}; // heterogeneous_checked_operation

// binary operations on primitive integer types

template<
    typename R,
    class F
>
struct checked_operation<R, F,
    typename std::enable_if<
        std::is_integral<R>::value
    >::type
>{
    ////////////////////////////////////////////////////
    // safe addition on primitive types

    struct add_impl_detail {
        // result unsigned
        constexpr static checked_result<R> add(
            const R t,
            const R u,
            std::false_type // R unsigned
        ){
            return
                // INT30-C. Ensure that unsigned integer operations do not wrap
                std::numeric_limits<R>::max() - u < t ?
                    F::template invoke<safe_numerics_error::positive_overflow_error>(
                        "addition result too large"
                    )
                :
                    checked_result<R>(t + u)
            ;
        }

        // result signed
        constexpr static checked_result<R> add(
            const R t,
            const R u,
            std::true_type // R signed
        ){
        // INT32-C. Ensure that operations on signed integers do not result in overflow
            return
                // INT32-C. Ensure that operations on signed integers do not result in overflow
                ((u > 0) && (t > (std::numeric_limits<R>::max() - u))) ?
                    F::template invoke<safe_numerics_error::positive_overflow_error>(
                        "addition result too large"
                    )
                :
                ((u < 0) && (t < (std::numeric_limits<R>::min() - u))) ?
                    F::template invoke<safe_numerics_error::negative_overflow_error>(
                        "addition result too low"
                    )
                :
                    checked_result<R>(t + u)
            ;
        }
    }; // add_impl_detail

    constexpr static checked_result<R>
    add(const R & t, const R & u){
        return add_impl_detail::add(t, u, std::is_signed<R>());
    }

    ////////////////////////////////////////////////////
    // safe subtraction on primitive types
    struct subtract_impl_detail {

        // result unsigned
        constexpr static checked_result<R> subtract(
            const R t,
            const R u,
            std::false_type // R is unsigned
        ){
            // INT30-C. Ensure that unsigned integer operations do not wrap
            return
                t < u ?
                    F::template invoke<safe_numerics_error::negative_overflow_error>(
                        "subtraction result cannot be negative"
                    )
                :
                    checked_result<R>(t - u)
            ;
        }

        // result signed
        constexpr static checked_result<R> subtract(
            const R t,
            const R u,
            std::true_type // R is signed
        ){ // INT32-C
            return
                // INT32-C. Ensure that operations on signed integers do not result in overflow
                ((u > 0) && (t < (std::numeric_limits<R>::min() + u))) ?
                    F::template invoke<safe_numerics_error::negative_overflow_error>(
                        "subtraction result overflows result type"
                    )
                :
                ((u < 0) && (t > (std::numeric_limits<R>::max() + u))) ?
                    F::template invoke<safe_numerics_error::positive_overflow_error>(
                        "subtraction result overflows result type"
                    )
                :
                    checked_result<R>(t - u)
            ;
        }

    }; // subtract_impl_detail

    constexpr static checked_result<R> subtract(const R & t, const R & u){
        return subtract_impl_detail::subtract(t, u, std::is_signed<R>());
    }

    ////////////////////////////////////////////////////
    // safe minus on primitive types
    struct minus_impl_detail {

        // result unsigned
        constexpr static checked_result<R> minus(
            const R t,
            std::false_type // R is unsigned
        ){
            return t > 0 ?
                    F::template invoke<safe_numerics_error::negative_overflow_error>(
                        "minus unsigned would be negative"
                    )
                :
                    // t == 0
                    checked_result<R>(0)
            ;
        }

        // result signed
        constexpr static checked_result<R> minus(
            const R t,
            std::true_type // R is signed
        ){ // INT32-C
            return t == std::numeric_limits<R>::min() ?
                F::template invoke<safe_numerics_error::positive_overflow_error>(
                    "subtraction result overflows result type"
                )
            :
                    checked_result<R>(-t)
            ;
        }

    }; // minus_impl_detail

    constexpr static checked_result<R> minus(const R & t){
        return minus_impl_detail::minus(t, std::is_signed<R>());
    }

    ////////////////////////////////////////////////////
    // safe multiplication on primitive types

    struct multiply_impl_detail {

        // result unsigned
        constexpr static checked_result<R> multiply(
            const R t,
            const R u,
            std::false_type,  // R is unsigned
            std::false_type   // !(sizeochecked_result<R>R) > sizeochecked_result<R>std::uintmax_t) / 2)

        ){
            // INT30-C
            // fast method using intermediate result guaranteed not to overflow
            // todo - replace std::uintmax_t with a size double the size of R
            using i_type = std::uintmax_t;
            return
                static_cast<i_type>(t) * static_cast<i_type>(u)
                > std::numeric_limits<R>::max() ?
                    F::template invoke<safe_numerics_error::positive_overflow_error>(
                        "multiplication overflow"
                    )
                :
                    checked_result<R>(t * u)
            ;
        }
        constexpr static checked_result<R> multiply(
            const R t,
            const R u,
            std::false_type,  // R is unsigned
            std::true_type    // (sizeochecked_result<R>R) > sizeochecked_result<R>std::uintmax_t) / 2)

        ){
            // INT30-C
            return
                u > 0 && t > std::numeric_limits<R>::max() / u ?
                    F::template invoke<safe_numerics_error::positive_overflow_error>(
                        "multiplication overflow"
                    )
                :
                    checked_result<R>(t * u)
            ;
        }

        // result signed
        constexpr static checked_result<R> multiply(
            const R t,
            const R u,
            std::true_type, // R is signed
            std::false_type // ! (sizeochecked_result<R>R) > (sizeochecked_result<R>std::intmax_t) / 2))

        ){
            // INT30-C
            // fast method using intermediate result guaranteed not to overflow
            // todo - replace std::intmax_t with a size double the size of R
            using i_type = std::intmax_t;
            return
                (
                    static_cast<i_type>(t) * static_cast<i_type>(u)
                    > static_cast<i_type>(std::numeric_limits<R>::max())
                ) ?
                    F::template invoke<safe_numerics_error::positive_overflow_error>(
                        "multiplication overflow"
                    )
                :
                (
                    static_cast<i_type>(t) * static_cast<i_type>(u)
                    < static_cast<i_type>(std::numeric_limits<R>::min())
                ) ?
                    F::template invoke<safe_numerics_error::negative_overflow_error>(
                        "multiplication overflow"
                    )
                :
                    checked_result<R>(t * u)
            ;
        }
        constexpr static checked_result<R> multiply(
            const R t,
            const R u,
            std::true_type,   // R is signed
            std::true_type    // (sizeochecked_result<R>R) > (sizeochecked_result<R>std::intmax_t) / 2))
        ){ // INT32-C
            return t > 0 ?
                u > 0 ?
                    t > std::numeric_limits<R>::max() / u ?
                        F::template invoke<safe_numerics_error::positive_overflow_error>(
                            "multiplication overflow"
                        )
                    :
                        checked_result<R>(t * u)
                : // u <= 0
                    u < std::numeric_limits<R>::min() / t ?
                        F::template invoke<safe_numerics_error::negative_overflow_error>(
                            "multiplication overflow"
                        )
                    :
                        checked_result<R>(t * u)
            : // t <= 0
                u > 0 ?
                    t < std::numeric_limits<R>::min() / u ?
                        F::template invoke<safe_numerics_error::negative_overflow_error>(
                            "multiplication overflow"
                        )
                    :
                        checked_result<R>(t * u)
                : // u <= 0
                    t != 0 && u < std::numeric_limits<R>::max() / t ?
                        F::template invoke<safe_numerics_error::positive_overflow_error>(
                            "multiplication overflow"
                        )
                    :
                        checked_result<R>(t * u)
            ;
        }
    }; // multiply_impl_detail

    constexpr static checked_result<R> multiply(const R & t, const R & u){
        return multiply_impl_detail::multiply(
            t,
            u,
            std::is_signed<R>(),
            std::integral_constant<
                bool,
                (sizeof(R) > sizeof(std::uintmax_t) / 2)
            >()
        );
    }

    ////////////////////////////////
    // safe division on unsafe types

    struct divide_impl_detail {
        constexpr static checked_result<R> divide(
            const R & t,
            const R & u,
            std::false_type // R is unsigned
        ){
            return t / u;
        }

        constexpr static checked_result<R> divide(
            const R & t,
            const R & u,
            std::true_type // R is signed
        ){
            return
                (u == -1 && t == std::numeric_limits<R>::min()) ?
                    F::template invoke<safe_numerics_error::positive_overflow_error>(
                        "result cannot be represented"
                    )
                :
                    checked_result<R>(t / u)
            ;
        }
    }; // divide_impl_detail

    // note that we presume that the size of R >= size of T
    constexpr static checked_result<R> divide(const R & t, const R & u){
        if(u == 0){
            return F::template invoke<safe_numerics_error::domain_error>(
                "divide by zero"
            );
        }
        return divide_impl_detail::divide(t, u, std::is_signed<R>());
    }

    ////////////////////////////////
    // safe modulus on unsafe types

    struct modulus_impl_detail {
        constexpr static checked_result<R> modulus(
            const R & t,
            const R & u,
            std::false_type // R is unsigned
        ){
            return t % u;
        }

        constexpr static checked_result<R> modulus(
            const R & t,
            const R & u,
            std::true_type // R is signed
        ){
            if(u >= 0)
                return t % u;
            checked_result<R> ux = checked::minus(u);
            if(ux.exception())
                return t;
            return t % static_cast<R>(ux);
        }
    }; // modulus_impl_detail

    constexpr static checked_result<R> modulus(const R & t, const R & u){
        if(0 == u)
            return F::template invoke<safe_numerics_error::domain_error>(
                "denominator is zero"
            );

        // why to we need abs here? the sign of the modulus is the sign of the
        // dividend. Consider -128 % -1  The result of this operation should be -1
        // but if I use t % u the x86 hardware uses the divide instruction
        // capturing the modulus as a side effect.  When it does this, it
        // invokes the operation -128 / -1 -> 128 which overflows a signed type
        // and provokes a hardware exception.  We can fix this using abs()
        // since -128 % -1 = -128 % 1 = 0
        return modulus_impl_detail::modulus(t, u, typename std::is_signed<R>::type());
    }

    ///////////////////////////////////
    // shift operations

    struct left_shift_integer_detail {

        #if 0
        // todo - optimize for gcc to exploit builtin
        /* for gcc compilers
        int __builtin_clz (unsigned int x)
              Returns the number of leading 0-bits in x, starting at the
              most significant bit position.  If x is 0, the result is undefined.
        */

        #ifndef __has_feature         // Optional of course.
          #define __has_feature(x) 0  // Compatibility with non-clang compilers.
        #endif

        template<typename T>
        constexpr unsigned int leading_zeros(const T & t){
            if(0 == t)
                return 0;
            #if __has_feature(builtin_clz)
                return  __builtin_clz(t);
            #else
            #endif
        }
        #endif

        // INT34-C C++

        // standard paragraph 5.8 / 2
        // The value of E1 << E2 is E1 left-shifted E2 bit positions;
        // vacated bits are zero-filled.
        constexpr static checked_result<R> left_shift(
            const R & t,
            const R & u,
            std::false_type // R is unsigned
        ){
            // the value of the result is E1 x 2^E2, reduced modulo one more than
            // the maximum value representable in the result type.

            // see 5.8 & 1
            // if right operand is
            // greater than or equal to the length in bits of the promoted left operand.
            if(
                safe_compare::greater_than(
                    u,
                    std::numeric_limits<R>::digits - utility::significant_bits(t)
                )
            ){
                // behavior is undefined
                return F::template invoke<safe_numerics_error::shift_too_large>(
                   "shifting left more bits than available is undefined behavior"
                );
            }
            return t << u;
        }

        constexpr static checked_result<R> left_shift(
            const R & t,
            const R & u,
            std::true_type // R is signed
        ){
            // and [E1] has a non-negative value
            if(t >= 0){
                // and E1 x 2^E2 is representable in the corresponding
                // unsigned type of the result type,

                // see 5.8 & 1
                // if right operand is
                // greater than or equal to the length in bits of the promoted left operand.
                if(
                    safe_compare::greater_than(
                        u,
                        std::numeric_limits<R>::digits - utility::significant_bits(t)
                    )
                ){
                    // behavior is undefined
                    return F::template invoke<safe_numerics_error::shift_too_large>(
                       "shifting left more bits than available"
                    );
                }
                else{
                    return t << u;
                }
            }
            // otherwise, the behavior is undefined.
            return F::template invoke<safe_numerics_error::negative_shift>(
               "shifting a negative value"
            );
        }

    }; // left_shift_integer_detail

    constexpr static checked_result<R> left_shift(
        const R & t,
        const R & u
    ){
        // INT34-C - Do not shift an expression by a negative number of bits

        // standard paragraph 5.8 & 1
        // if the right operand is negative
        if(u == 0){
            return t;
        }
        if(u < 0){
            return F::template invoke<safe_numerics_error::negative_shift>(
               "shifting negative amount"
            );
        }
        if(u > std::numeric_limits<R>::digits){
            // behavior is undefined
            return F::template invoke<safe_numerics_error::shift_too_large>(
               "shifting more bits than available"
            );
        }
        return left_shift_integer_detail::left_shift(t, u, std::is_signed<R>());
    }

    // right shift

    struct right_shift_integer_detail {

        // INT34-C C++

        // standard paragraph 5.8 / 3
        // The value of E1 << E2 is E1 left-shifted E2 bit positions;
        // vacated bits are zero-filled.
        constexpr static checked_result<R> right_shift(
            const R & t,
            const R & u,
            std::false_type // T is unsigned
        ){
            // the value of the result is the integral part of the
            // quotient of E1/2E2
            return t >> u;
        }

        constexpr static checked_result<R> right_shift(
            const R & t,
            const R & u,
            std::true_type  // T is signed;
        ){
        if(t < 0){
            // note that the C++ standard considers this case is "implemenation
            // defined" rather than "undefined".
            return F::template invoke<safe_numerics_error::negative_value_shift>(
                "shifting a negative value"
            );
         }

         // the value is the integral part of E1 / 2^E2,
         return t >> u;
        }
    }; // right_shift_integer_detail

    constexpr static checked_result<R> right_shift(
        const R & t,
        const R & u
    ){
        // INT34-C - Do not shift an expression by a negative number of bits

        // standard paragraph 5.8 & 1
        // if the right operand is negative
        if(u < 0){
            return F::template invoke<safe_numerics_error::negative_shift>(
               "shifting negative amount"
            );
        }
        if(u > std::numeric_limits<R>::digits){
            // behavior is undefined
            return F::template invoke<safe_numerics_error::shift_too_large>(
               "shifting more bits than available"
            );
        }
        return right_shift_integer_detail::right_shift(t, u ,std::is_signed<R>());
    }

    ///////////////////////////////////
    // bitwise operations

    // INT13-C Note: We don't enforce recommendation as acually written
    // as it would break too many programs.  Specifically, we permit signed
    // integer operands.

    constexpr static checked_result<R> bitwise_or(const R & t, const R & u){
        using namespace boost::safe_numerics::utility;
        const unsigned int result_size
            = std::max(significant_bits(t), significant_bits(u));

        if(result_size > bits_type<R>::value){
            return F::template invoke<safe_numerics_error::positive_overflow_error>(
                "result type too small to hold bitwise or"
            );
        }
        return t | u;
    }

    constexpr static checked_result<R> bitwise_xor(const R & t, const R & u){
        using namespace boost::safe_numerics::utility;
        const unsigned int result_size
            = std::max(significant_bits(t), significant_bits(u));

        if(result_size > bits_type<R>::value){
            return F::template invoke<safe_numerics_error::positive_overflow_error>(
                "result type too small to hold bitwise or"
            );
        }
        return t ^ u;
    }

    constexpr static checked_result<R> bitwise_and(const R & t, const R & u){
        using namespace boost::safe_numerics::utility;
        const unsigned int result_size
            = std::min(significant_bits(t), significant_bits(u));

        if(result_size > bits_type<R>::value){
            return F::template invoke<safe_numerics_error::positive_overflow_error>(
                "result type too small to hold bitwise and"
            );
        }
        return t & u;
    }

    constexpr static checked_result<R> bitwise_not(const R & t){
        using namespace boost::safe_numerics::utility;

        if(significant_bits(t) > bits_type<R>::value){
            return F::template invoke<safe_numerics_error::positive_overflow_error>(
                "result type too small to hold bitwise inverse"
            );
        }
        return ~t;
    }

}; // checked_operation
} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CHECKED_INTEGER_HPP
