// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DERIVED_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DERIVED_HPP_INCLUDED

namespace boost {
namespace type_erasure {

/**
 * A metafunction which returns the full @ref any type,
 * when given any of its base classes.  This is primarily
 * intended to be used when implementing @ref concept_interface.
 *
 * \see rebind_any, as_param
 */
template<class T>
struct derived
{
#ifdef BOOST_TYPE_ERASURE_DOXYGEN
    typedef detail::unspecified type;
#else
    typedef typename T::_boost_type_erasure_derived_type type;
#endif
};

#ifndef BOOST_NO_CXX11_TEMPLATE_ALIASES

template<class T>
using derived_t = typename T::_boost_type_erasure_derived_type;

#endif

}
}

#endif
