// Copyright 2018-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_CONVERT_INTEGER_HPP
#define BOOST_HISTOGRAM_DETAIL_CONVERT_INTEGER_HPP

#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T, class U>
using convert_integer =
    std::conditional_t<std::is_integral<std::decay_t<T>>::value, U, T>;

}
} // namespace histogram
} // namespace boost

#endif
