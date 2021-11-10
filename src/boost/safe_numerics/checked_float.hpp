#ifndef BOOST_NUMERIC_CHECKED_FLOAT_HPP
#define BOOST_NUMERIC_CHECKED_FLOAT_HPP

//  Copyright (c) 2017 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// contains operation implementation of arithmetic operators
// on built-in floating point types.  The default implementation is to just
// invoke the operation with no checking.  These are overloaded
// for specific types such as integer, etc.

#include <type_traits> // std::is_floating_point, make_unsigned

namespace boost {
namespace safe_numerics {
namespace checked {

////////////////////////////////////////////////////
// layer 0 - implement safe operations for floating

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
        && std::is_floating_point<T>::value
    >::type
>{
    constexpr static checked_result<R>
    cast(const T & t) noexcept {
        return t;
    };
}; // checked_unary_operation

template<
    typename R,
    R Min,
    R Max,
    typename T,
    class
    F
>
struct heterogeneous_checked_operation<
    R,
    Min,
    Max,
    T,
    F,
    typename std::enable_if<
        std::is_floating_point<R>::value
        && std::is_integralt<T>::value
    >::type
>{
    constexpr static checked_result<R>
    cast(const T & t) noexcept {
        return t;
    };
}; // checked_unary_operation

template<typename R, typename T, typename U>
struct checked_operation<R, T, U, F,
    typename std::enable_if<
        std::is_floating_point<R>::value
    >::type
>{
    constexpr static checked_result<R> cast(const T & t) {
        return
            cast_impl_detail::cast_impl(
                t,
                std::is_signed<R>(),
                std::is_signed<T>()
            );
    }
    constexpr static checked_result<R> add(const T & t, const U & u) {
        return t + u;
    }

    constexpr static checked_result<R> subtract(
        const T & t,
        const U & u
    ) {
        return t - u;
    }

    constexpr static checked_result<R> multiply(
        const T & t,
        const U & u
    ) noexcept {
        return t * u;
    }

    constexpr static checked_result<R> divide(
        const T & t,
        const U & u
    ) noexcept {
        return t / u;
    }

    constexpr static checked_result<R> modulus(
        const T & t,
        const U & u
    ) noexcept {
        return t % u;
    }

    constexpr static bool less_than(const T & t, const U & u) noexcept {
        return t < u;
    }

    constexpr static bool greater_than(const T & t, const U & u) noexcept {
        return t > u;
    }

    constexpr static bool equal(const T & t, const U & u) noexcept {
        return t < u;
    }

}; // checked_binary_operation

template<class R, class T, class U>
typename std::enable_if<
    std::is_floating_point<R>::value
    && std::is_floating_point<T>::value
    && std::is_floating_point<U>::value,
    checked_result<R>
>::type
constexpr inline bool less_than(const T & t, const U & u) noexcept {
    return t < u;
}

template<class R, class T, class U>
typename std::enable_if<
    std::is_floating_point<R>::value
    && std::is_floating_point<T>::value
    && std::is_floating_point<U>::value,
    checked_result<R>
>::type
constexpr inline bool equal(const T & t, const U & u) noexcept {
    return t < u;
}

template<class R, class T, class U>
typename std::enable_if<
    std::is_floating_point<R>::value
    && std::is_floating_point<T>::value
    && std::is_floating_point<U>::value,
    checked_result<R>
>::type
constexpr inline checked_result<R> left_shift(const T & t, const U & u) noexcept {
    return t << u;
}

template<class R, class T, class U>
typename std::enable_if<
    std::is_floating_point<R>::value
    && std::is_floating_point<T>::value
    && std::is_floating_point<U>::value,
    checked_result<R>
>::type
constexpr inline checked_result<R> right_shift(const T & t, const U & u) noexcept {
    return t >> u;
}

template<class R, class T, class U>
typename std::enable_if<
    std::is_floating_point<R>::value
    && std::is_floating_point<T>::value
    && std::is_floating_point<U>::value,
    checked_result<R>
>::type
constexpr inline checked_result<R> bitwise_or(const T & t, const U & u) noexcept {
    return t | u;
}

template<class R, class T, class U>
typename std::enable_if<
    std::is_floating_point<R>::value
    && std::is_floating_point<T>::value
    && std::is_floating_point<U>::value,
    checked_result<R>
>::type
constexpr inline checked_result<R> bitwise_xor(const T & t, const U & u) noexcept {
    return t ^ u;
}

template<class R, class T, class U>
typename std::enable_if<
    std::is_floating_point<R>::value
    && std::is_floating_point<T>::value
    && std::is_floating_point<U>::value,
    checked_result<R>
>::type
constexpr inline checked_result<R> bitwise_and(const T & t, const U & u) noexcept {
    return t & u;
}

} // checked
} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CHECKED_DEFAULT_HPP

