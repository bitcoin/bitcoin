// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file tags/tagged.hpp
/// \brief Defines the tagged class

#ifndef BOOST_BIMAP_TAGS_TAGGED_HPP
#define BOOST_BIMAP_TAGS_TAGGED_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

namespace boost {
namespace bimaps {

/// \brief A light non-invasive idiom to tag a type.
/**

There are a lot of ways of tagging a type. The standard library for example
defines tags (empty structs) that are then inherited by the tagged class. To
support built-in types and other types that simple cannot inherit from the
tag, the standard builds another level of indirection. An example of this is
the type_traits metafunction. This approach is useful if the tags are intended
to be used in the library internals, and if the user does not have to create
new tagged types often.

Boost.MultiIndex is an example of a library that defines a tagged idiom that
is better suited to the user. As an option, in the indexed by declaration
of a multi-index container a user can \b attach a tag to each index, so it
can be referred by it instead of by the index number. It is a very user
friendly way of specifying a tag but is very invasive from the library writer's
point of view. Each index must now support this additional parameter. Maybe
not in the case of the multi-index container, but in simpler classes
the information of the tags is used by the father class rather than by the
tagged types.

\b tagged is a light non-invasive idiom to tag a type. It is very intuitive
and user-friendly. With the use of the defined metafunctions the library
writer can enjoy the coding too.

                                                                            **/

namespace tags {

/// \brief The tag holder
/**

The idea is to add a level of indirection to the type being tagged. With this
class you wrapped a type and apply a tag to it. The only thing to remember is
that if you write

\code
typedef tagged<type,tag> taggedType;
\endcode

Then instead to use directly the tagged type, in order to access it you have
to write \c taggedType::value_type. The tag can be obtained using \c taggedType::tag.
The idea is not to use this metadata directly but rather using the metafunctions
that are defined in the support namespace. With this metafunctions you can work
with tagged and untagged types in a consistent way. For example, the following
code is valid:

\code
BOOST_STATIC_ASSERT( is_same< value_type_of<taggedType>, value_type_of<type> >::value );
\endcode

The are other useful metafunctions there too.
See also value_type_of, tag_of, is_tagged, apply_to_value_type.

\ingroup tagged_group
                                                                                    **/
template< class Type, class Tag >
struct tagged
{
    typedef Type value_type;
    typedef Tag tag;
};

} // namespace tags
} // namespace bimaps
} // namespace boost

/** \namespace boost::bimaps::tags::support
\brief Metafunctions to work with tagged types.

This metafunctions aims to make easier the manage of tagged types. They are all mpl
compatible metafunctions and can be used with lambda expressions.
The metafunction value_type_of and tag_of get the data in a tagged type in a secure
and consistent way.
default_tagged and overwrite_tagged allows to work with the tag of a tagged type,
and apply_to_value_type is a higher order metafunction that allow the user to change
the type of a TaggedType.
                                                                                    **/

#endif // BOOST_BIMAP_TAGS_TAGGED_HPP




