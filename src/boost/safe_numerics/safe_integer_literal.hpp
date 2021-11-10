#ifndef BOOST_NUMERIC_SAFE_INTEGER_LITERAL_HPP
#define BOOST_NUMERIC_SAFE_INTEGER_LITERAL_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cstdint> // for intmax_t/uintmax_t
#include <iosfwd>
#include <type_traits> // conditional, enable_if
#include <boost/mp11/utility.hpp>

#include "utility.hpp"
#include "safe_integer.hpp"
#include "checked_integer.hpp"

namespace boost {
namespace safe_numerics {

template<typename T, T N, class P, class E>
class safe_literal_impl;

template<typename T, T N, class P, class E>
struct is_safe<safe_literal_impl<T, N, P, E> > : public std::true_type
{};

template<typename T, T N, class P, class E>
struct get_promotion_policy<safe_literal_impl<T, N, P, E> > {
    using type = P;
};

template<typename T, T N, class P, class E>
struct get_exception_policy<safe_literal_impl<T, N, P, E> > {
    using type = E;
};
template<typename T, T N, class P, class E>
struct base_type<safe_literal_impl<T, N, P, E> > {
    using type = T;
};

template<typename T, T N, class P, class E>
constexpr T base_value(
    const safe_literal_impl<T, N, P, E> &
) {
    return N;
}

template<typename CharT, typename Traits, typename T, T N, class P, class E>
inline std::basic_ostream<CharT, Traits> & operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const safe_literal_impl<T, N, P, E> &
){
    return os << (
        (std::is_same<T, signed char>::value
        || std::is_same<T, unsigned char>::value
        ) ?
            static_cast<int>(N)
        :
            N
    );
}

template<typename T, T N, class P, class E>
class safe_literal_impl {

    template<
        class CharT,
        class Traits
    >
    friend std::basic_ostream<CharT, Traits> & operator<<(
        std::basic_ostream<CharT, Traits> & os,
        const safe_literal_impl &
    ){
        return os << (
            (::std::is_same<T, signed char>::value
            || ::std::is_same<T, unsigned char>::value
            || ::std::is_same<T, wchar_t>::value
            ) ?
                static_cast<int>(N)
            :
                N
        );
    };

public:

    ////////////////////////////////////////////////////////////
    // constructors
    // default constructor
    constexpr safe_literal_impl(){}

    /////////////////////////////////////////////////////////////////
    // casting operators for intrinsic integers
    // convert to any type which is not safe.  safe types need to be
    // excluded to prevent ambiguous function selection which
    // would otherwise occur
    template<
        class R,
        typename std::enable_if<
            ! boost::safe_numerics::is_safe<R>::value,
            int
        >::type = 0
    >
    constexpr operator R () const {
        // if static values don't overlap, the program can never function
        #if 1
        constexpr const interval<R> r_interval;
        static_assert(
            ! r_interval.excludes(N),
            "safe type cannot be constructed with this type"
        );
        #endif

        return validate_detail<
            R,
            std::numeric_limits<R>::min(),
            std::numeric_limits<R>::max(),
            E
        >::return_value(*this);
    }

    // non mutating unary operators
    constexpr safe_literal_impl<T, N, P, E> operator+() const { // unary plus
        return safe_literal_impl<T, N, P, E>();
    }
    // after much consideration, I've permitted the resulting value of a unary
    // - to change the type in accordance with the promotion policy.
    // The C++ standard does invoke integral promotions so it's changing the type as well.

    /*  section 5.3.1 &8 of the C++ standard
    The operand of the unary - operator shall have arithmetic or unscoped
    enumeration type and the result is the negation of its operand. Integral
    promotion is performed on integral or enumeration operands. The negative
    of an unsigned quantity is computed by subtracting its value from 2n,
    where n is the number of bits in the promoted operand. The type of the
    result is the type of the promoted operand.
    */
    template<
        typename Tx, Tx Nx, typename = std::enable_if_t<! checked::minus(Nx).exception()>
    >
    constexpr auto minus_helper() const {
        return safe_literal_impl<Tx, -N, P, E>();
    }

    constexpr auto operator-() const { // unary minus
        return minus_helper<T, N>();
    }

    /*   section 5.3.1 &10 of the C++ standard
    The operand of ~ shall have integral or unscoped enumeration type; 
    the result is the ones’ complement of its operand. Integral promotions 
    are performed. The type of the result is the type of the promoted operand.
    constexpr safe_literal_impl<T, checked::bitwise_not(N), P, E> operator~() const { // invert bits
        return safe_literal_impl<T, checked::bitwise_not(N), P, E>();
    }
    */
    template<
        typename Tx, Tx Nx, typename = std::enable_if_t<! checked::bitwise_not(Nx).exception()>
    >
    constexpr auto not_helper() const {
        return safe_literal_impl<Tx, ~N, P, E>();
    }

    constexpr auto operator~() const { // unary minus
        return not_helper<T, N>();
    }
};

template<
    std::intmax_t N,
    class P = void,
    class E = void
>
using safe_signed_literal = safe_literal_impl<
    typename utility::signed_stored_type<N, N>,
    N,
    P,
    E
>;

template<
    std::uintmax_t N,
    class P = void,
    class E = void
>
using safe_unsigned_literal = safe_literal_impl<
    typename utility::unsigned_stored_type<N, N>,
    N,
    P,
    E
>;

template<
    class T,
    T N,
    class P = void,
    class E = void,
    typename std::enable_if<
        std::is_signed<T>::value,
        int
    >::type = 0
>
constexpr auto make_safe_literal_impl() {
    return boost::safe_numerics::safe_signed_literal<N, P, E>();
}

template<
    class T,
    T N,
    class P = void,
    class E = void,
    typename std::enable_if<
        ! std::is_signed<T>::value,
        int
    >::type = 0
>
constexpr auto inline make_safe_literal_impl() {
    return boost::safe_numerics::safe_unsigned_literal<N, P, E>();
}

} // safe_numerics
} // boost

#define make_safe_literal(n, P, E)  \
    boost::safe_numerics::make_safe_literal_impl<decltype(n), n, P, E>()

/////////////////////////////////////////////////////////////////
// numeric limits for safe_literal etc.

#include <limits>

namespace std {

template<
    typename T,
    T N,
    class P,
    class E
>
class numeric_limits<boost::safe_numerics::safe_literal_impl<T, N, P, E> >
    : public std::numeric_limits<T>
{
    using SL = boost::safe_numerics::safe_literal_impl<T, N, P, E>;
public:
    constexpr static SL lowest() noexcept {
        return SL();
    }
    constexpr static SL min() noexcept {
        return SL();
    }
    constexpr static SL max() noexcept {
        return SL();
    }
};

} // std

#endif // BOOST_NUMERIC_SAFE_INTEGER_LITERAL_HPP
