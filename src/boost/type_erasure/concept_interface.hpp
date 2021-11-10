// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_CONCEPT_INTERFACE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_CONCEPT_INTERFACE_HPP_INCLUDED

namespace boost {
namespace type_erasure {

/**
 * The @ref concept_interface class can be specialized to
 * add behavior to an @ref any.  An @ref any inherits from
 * all the relevant specializations of @ref concept_interface.
 *
 * @ref concept_interface can be specialized for either
 * primitive or composite concepts.  If a concept @c C1
 * contains another concept @c C2, then the library guarantees
 * that the specialization of @ref concept_interface for
 * @c C2 is a base class of the specialization for @c C1.
 * This means that @c C1 can safely override members of @c C2.
 *
 * @ref concept_interface may only be specialized for user-defined
 * concepts.  The library owns the specializations of its own
 * built in concepts.
 *
 * \tparam Concept The concept that we're specializing
 *         @ref concept_interface for.  One of its
 *         placeholders should be @c ID.
 * \tparam Base The base of this class.  Specializations of @ref
 *         concept_interface must inherit publicly from this type.
 * \tparam ID The placeholder representing this type.
 * \tparam Enable A dummy parameter that can be used for SFINAE.
 *
 * The metafunctions @ref derived, @ref rebind_any, and @ref as_param
 * (which can be applied to @c Base) are useful for determining the
 * argument and return types of functions defined in @ref concept_interface.
 *
 * For dispatching the function use \call.
 */
template<class Concept, class Base, class ID, class Enable = void>
struct concept_interface : Base {};

}
}

#endif
