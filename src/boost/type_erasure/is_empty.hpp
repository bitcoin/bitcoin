// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_IS_EMPTY_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_IS_EMPTY_HPP_INCLUDED

#include <boost/type_erasure/detail/access.hpp>

namespace boost {
namespace type_erasure {

/** Returns true for an empty @ref any. */
template<class T>
bool is_empty(const T& arg) {
    return ::boost::type_erasure::detail::access::data(arg).data == 0;
}

}
}

#endif
