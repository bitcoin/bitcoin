// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_STATIC_BINDING_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_STATIC_BINDING_HPP_INCLUDED

namespace boost {
namespace type_erasure {

/**
 * Represents a mapping from placeholders to the actual types
 * that they bind to.
 *
 * \pre @c Map must be an MPL map whose keys are placeholders.
 */
template<class Map>
struct static_binding { typedef Map map_type; };

/**
 * A convenience function to prevent constructor calls
 * from being parsed as function declarations.
 */
template<class Map>
static_binding<Map> make_binding() { return static_binding<Map>(); }

}
}

#endif
