#ifndef BOOST_NUMERIC_CONCEPT_NUMERIC_HPP
#define BOOST_NUMERIC_CONCEPT_NUMERIC_HPP

//  Copyright (c) 2021 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <limits>
#include <cstdint>
#include <type_traits>

namespace boost {
namespace safe_numerics {

template<class T>
using Numeric = std::integral_constant<bool, std::numeric_limits<T>::is_specialized>;

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CONCEPT_NUMERIC_HPP
