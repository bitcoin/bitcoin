// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_SAME_TYPE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_SAME_TYPE_HPP_INCLUDED

namespace boost {
namespace type_erasure {

/**
 * A built in concept that indicates that two
 * types are the same.  Either T or U or both
 * can be placeholders.
 *
 * \warning Any number of instances of @ref deduced
 * can be connected with @ref same_type, but there
 * should be at most one regular placeholder in
 * the group. same_type<_a, _b> is not allowed.
 * The reason for this is that the library needs
 * to normalize all the placeholders, and in this
 * context there is no way to decide whether to
 * use @ref _a or @ref _b.
 */
template<class T, class U>
struct same_type {};

}
}

#endif
