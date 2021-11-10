// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_PLACEHOLDERS_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_PLACEHOLDERS_HPP_INCLUDED

namespace boost {
namespace type_erasure {

/**
 * Placeholders are used heavily throughout the library.
 * Every placeholder must derive from @ref placeholder.
 * The library provides a number of placeholders,
 * out of the box, but you are welcome to define your own,
 * if you want more descriptive names.  The placeholder
 * @ref _self is special in that it is used as the default
 * wherever possible.
 *
 * What exactly is a placeholder?  Placeholders act as
 * a substitute for template parameters in concepts.
 * The library automatically replaces all the placeholders
 * used in a concept with the actual types involved when
 * it stores an object in an @ref any.
 *
 * For example, in the following,
 *
 * @code
 * any<copy_constructible<_a>, _a> x(1);
 * @endcode
 *
 * The library sees that we're constructing an @ref any
 * that uses the @ref _a placeholder with an @c int.
 * Thus it binds @ref _a to int and instantiates
 * @ref copy_constructible "copy_constructible<int>".
 *
 * When there are multiple placeholders involved, you
 * will have to use @ref tuple, or pass the bindings
 * explicitly, but the substitution still works the
 * same way.
 */
struct placeholder {
    /// INTERNAL ONLY
    typedef void _boost_type_erasure_is_placeholder;
};

struct _a : placeholder {};
struct _b : placeholder {};
struct _c : placeholder {};
struct _d : placeholder {};
struct _e : placeholder {};
struct _f : placeholder {};
struct _g : placeholder {};

/**
 * \brief The default placeholder
 *
 * @ref _self is the default @ref placeholder used
 * by @ref any.  It should be used as a default
 * by most concepts, so using concepts with no
 * explicit arguments will "just work" as much as
 * possible.
 */
struct _self : placeholder {};

}
}

#endif
