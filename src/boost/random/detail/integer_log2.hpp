/* boost random/detail/integer_log2.hpp header file
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_DETAIL_INTEGER_LOG2_HPP
#define BOOST_RANDOM_DETAIL_INTEGER_LOG2_HPP

#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/integer/integer_log2.hpp>

namespace boost {
namespace random {
namespace detail {

#if !defined(BOOST_NO_CXX11_CONSTEXPR)
#define BOOST_RANDOM_DETAIL_CONSTEXPR constexpr
#elif defined(BOOST_MSVC)
#define BOOST_RANDOM_DETAIL_CONSTEXPR __forceinline
#elif defined(__GNUC__) && __GNUC__ >= 4
#define BOOST_RANDOM_DETAIL_CONSTEXPR inline __attribute__((__const__)) __attribute__((__always_inline__))
#else
#define BOOST_RANDOM_DETAIL_CONSTEXPR inline
#endif

template<int Shift>
struct integer_log2_impl
{
#if defined(BOOST_NO_CXX11_CONSTEXPR)
    template<class T>
    BOOST_RANDOM_DETAIL_CONSTEXPR static int apply(T t, int accum)
    {
        int update = ((t >> Shift) != 0) * Shift;
        return integer_log2_impl<Shift / 2>::apply(t >> update, accum + update);
    }
#else
    template<class T>
    BOOST_RANDOM_DETAIL_CONSTEXPR static int apply2(T t, int accum, int update)
    {
        return integer_log2_impl<Shift / 2>::apply(t >> update, accum + update);
    }

    template<class T>
    BOOST_RANDOM_DETAIL_CONSTEXPR static int apply(T t, int accum)
    {
        return apply2(t, accum, ((t >> Shift) != 0) * Shift);
    }
#endif
};

template<>
struct integer_log2_impl<1>
{
    template<class T>
    BOOST_RANDOM_DETAIL_CONSTEXPR static int apply(T t, int accum)
    {
        return int(t >> 1) + accum;
    }
};

template<class T>
BOOST_RANDOM_DETAIL_CONSTEXPR int integer_log2(T t)
{
    return integer_log2_impl<
        ::boost::detail::max_pow2_less<
            ::std::numeric_limits<T>::digits, 4
        >::value
    >::apply(t, 0);
}

} // namespace detail
} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_DETAIL_INTEGER_LOG2_HPP
