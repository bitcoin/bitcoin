#ifndef BOOST_NUMERIC_SAFE_COMMON_HPP
#define BOOST_NUMERIC_SAFE_COMMON_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <type_traits>

namespace boost {
namespace safe_numerics {

// default implementations for required meta-functions
template<typename T>
struct is_safe : public std::false_type
{};

template<typename T>
struct base_type {
    using type = T;
};

template<class T>
constexpr const typename base_type<T>::type & base_value(const T & t) {
    return static_cast<const typename base_type<T>::type & >(t);
}

template<typename T>
struct get_promotion_policy {
    using type = void;
};

template<typename T>
struct get_exception_policy {
    using type = void;
};


} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_SAFE_COMMON_HPP
