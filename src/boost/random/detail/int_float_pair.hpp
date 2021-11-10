/* boost random/detail/int_float_pair.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Copyright Steven Watanabe 2010-2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_DETAIL_INT_FLOAT_PAIR_HPP
#define BOOST_RANDOM_DETAIL_INT_FLOAT_PAIR_HPP

#include <utility>
#include <boost/integer.hpp>
#include <boost/integer/integer_mask.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/detail/signed_unsigned_tools.hpp>
#include <boost/random/detail/integer_log2.hpp>

namespace boost {
namespace random {
namespace detail {

template<class Engine>
inline typename boost::make_unsigned<typename Engine::result_type>::type
generate_one_digit(Engine& eng, std::size_t bits)
{
    typedef typename Engine::result_type base_result;
    typedef typename boost::make_unsigned<base_result>::type base_unsigned;
    
    base_unsigned range =
        detail::subtract<base_result>()((eng.max)(), (eng.min)());
    base_unsigned y0_mask = (base_unsigned(2) << (bits - 1)) - 1;
    base_unsigned y0 = (range + 1) & ~y0_mask;
    base_unsigned u;
    do {
        u = detail::subtract<base_result>()(eng(), (eng.min)());
    } while(y0 != 0 && u > base_unsigned(y0 - 1));
    return u & y0_mask;
}

template<class RealType, std::size_t w, class Engine>
std::pair<RealType, int> generate_int_float_pair(Engine& eng, boost::true_type)
{
    typedef typename Engine::result_type base_result;
    typedef typename boost::make_unsigned<base_result>::type base_unsigned;
    
    base_unsigned range =
        detail::subtract<base_result>()((eng.max)(), (eng.min)());
    
    std::size_t m =
        (range == (std::numeric_limits<base_unsigned>::max)()) ?
            std::numeric_limits<base_unsigned>::digits :
            detail::integer_log2(range + 1);
            
    int bucket = 0;
    // process as many full digits as possible into the int part
    for(std::size_t i = 0; i < w/m; ++i) {
        base_unsigned u = generate_one_digit(eng, m);
        bucket = (bucket << m) | u;
    }
    RealType r;

    const std::size_t digits = std::numeric_limits<RealType>::digits;
    {
        base_unsigned u = generate_one_digit(eng, m);
        base_unsigned mask = (base_unsigned(1) << (w%m)) - 1;
        bucket = (bucket << (w%m)) | (mask & u);
        const RealType mult = RealType(1)/RealType(base_unsigned(1) << (m - w%m));
        // zero out unused bits
        if (m - w%m > digits) {
            u &= ~(base_unsigned(1) << (m - digits));
        }
        r = RealType(u >> (w%m)) * mult;
    }
    for(std::size_t i = m - w%m; i + m < digits; i += m) {
        base_unsigned u = generate_one_digit(eng, m);
        r += u;
        r *= RealType(0.5)/RealType(base_unsigned(1) << (m - 1));
    }
    if (m - w%m < digits)
    {
        const std::size_t remaining = (digits - m + w%m) % m;
        base_unsigned u = generate_one_digit(eng, m);
        r += u & ((base_unsigned(2) << (remaining - 1)) - 1);
        const RealType mult = RealType(0.5)/RealType(base_unsigned(1) << (remaining - 1));
        r *= mult;
    }
    return std::make_pair(r, bucket);
}

template<class RealType, std::size_t w, class Engine>
inline std::pair<RealType, int> generate_int_float_pair(Engine& eng, boost::false_type)
{
    int bucket = uniform_int_distribution<>(0, (1 << w) - 1)(eng);
    RealType r = uniform_01<RealType>()(eng);
    return std::make_pair(r, bucket);
}

template<class RealType, std::size_t w, class Engine>
inline std::pair<RealType, int> generate_int_float_pair(Engine& eng)
{
    typedef typename Engine::result_type base_result;
    return generate_int_float_pair<RealType, w>(eng,
        boost::is_integral<base_result>());
}

} // namespace detail
} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_DETAIL_INT_FLOAT_PAIR_HPP
