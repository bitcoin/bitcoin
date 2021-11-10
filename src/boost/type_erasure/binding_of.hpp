// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_BINDING_OF_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_BINDING_OF_HPP_INCLUDED

#include <boost/type_erasure/detail/access.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/binding.hpp>

namespace boost {
namespace type_erasure {

/**
 * \return The type bindings of an @ref any.
 *
 * \throws Nothing.
 */
template<class Concept, class T>
const binding<Concept>& binding_of(const any<Concept, T>& arg)
{
    return ::boost::type_erasure::detail::access::table(arg);
}

}
}

#endif
