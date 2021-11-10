//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_DETAIL_CLAMP_HPP
#define BOOST_BEAST_CORE_DETAIL_CLAMP_HPP

#include <cstdlib>
#include <limits>
#include <type_traits>

namespace boost {
namespace beast {
namespace detail {

template<class UInt>
static
std::size_t
clamp(UInt x)
{
    if(x >= (std::numeric_limits<std::size_t>::max)())
        return (std::numeric_limits<std::size_t>::max)();
    return static_cast<std::size_t>(x);
}

template<class UInt>
static
std::size_t
clamp(UInt x, std::size_t limit)
{
    if(x >= limit)
        return limit;
    return static_cast<std::size_t>(x);
}

// return `true` if x + y > z, which are unsigned
template<
    class U1, class U2, class U3>
constexpr
bool
sum_exceeds(U1 x, U2 y, U3 z)
{
    static_assert(
        std::is_unsigned<U1>::value &&
        std::is_unsigned<U2>::value &&
        std::is_unsigned<U3>::value, "");
    return y > z || x > z - y;
}

} // detail
} // beast
} // boost

#endif
