//
// Copyright (c) Chris Glover, 2016.
//
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_TYPE_INDEX_RUNTIME_CAST_DETAIL_RUNTIME_CAST_IMPL_HPP
#define BOOST_TYPE_INDEX_RUNTIME_CAST_DETAIL_RUNTIME_CAST_IMPL_HPP

/// \file runtime_cast_impl.hpp
/// \brief Contains the overload of boost::typeindex::runtime_cast for
/// pointer types.
///
/// boost::typeindex::runtime_cast can be used to emulate dynamic_cast
/// functionality on platorms that don't provide it or should the user
/// desire opt in functionality instead of enabling it system wide.

#include <boost/type_index.hpp>
#include <boost/type_traits/integral_constant.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace typeindex {

namespace detail {

template<typename T, typename U>
T* runtime_cast_impl(U* u, boost::true_type) BOOST_NOEXCEPT {
    return u;
}

template<typename T, typename U>
T const* runtime_cast_impl(U const* u, boost::true_type) BOOST_NOEXCEPT {
    return u;
}

template<typename T, typename U>
T* runtime_cast_impl(U* u, boost::false_type) BOOST_NOEXCEPT {
    return const_cast<T*>(static_cast<T const*>(
        u->boost_type_index_find_instance_(boost::typeindex::type_id<T>())
    ));
}

template<typename T, typename U>
T const* runtime_cast_impl(U const* u, boost::false_type) BOOST_NOEXCEPT {
    return static_cast<T const*>(u->boost_type_index_find_instance_(boost::typeindex::type_id<T>()));
}

} // namespace detail

}} // namespace boost::typeindex

#endif // BOOST_TYPE_INDEX_RUNTIME_CAST_DETAIL_RUNTIME_CAST_IMPL_HPP
