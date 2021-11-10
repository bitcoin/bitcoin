// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DETAIL_IS_PLACEHOLDER_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_IS_PLACEHOLDER_HPP_INCLUDED

#include <boost/mpl/bool.hpp>

namespace boost {

namespace type_erasure {

#ifdef BOOST_TYPE_ERASURE_DOXYGEN

/** A metafunction that indicates whether a type is a @ref placeholder. */
template<class T>
struct is_placeholder {};

#else

template<class T, class Enable = void>
struct is_placeholder : ::boost::mpl::false_ {};

template<class T>
struct is_placeholder<T, typename T::_boost_type_erasure_is_placeholder> :
    ::boost::mpl::true_ {};

#endif

}
}

#endif
