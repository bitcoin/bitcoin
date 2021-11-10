// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/member_at.hpp
/// \brief Defines the tags for the member_at::side idiom

#ifndef BOOST_BIMAP_RELATION_MEMBER_AT_HPP
#define BOOST_BIMAP_RELATION_MEMBER_AT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

namespace boost {
namespace bimaps {
namespace relation {

/// \brief member_at::side idiom to access relation values and types using metaprogramming. 
/**

This tags are used to specify which member you want to access when using a metafunction over
a symmetrical type. The idea is to be able to write code like:

\code
result_of::get<member_at::left,relation>::type data = get<member_at::left>(rel);
\endcode

The relation class supports this idiom even when the elements are tagged. This is useful
because a user can decide to start tagging in any moment of the development.

See also member_with_tag, is_tag_of_member_at_left, is_tag_of_member_at_right, get
value_type_of, pair_by, pair_type_by.

\ingroup relation_group
                                                                                        **/
namespace member_at {

    /// \brief Member at left tag
    /**
    See also member_at, right.
                                            **/

    struct left  {};

    /// \brief Member at right tag
    /**
    See also member_at, left.
                                            **/

    struct right {};

    /// \brief Member info tag
    /**
    See also member_at, left, right.
                                            **/

    struct info  {};

}

} // namespace relation
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_RELATION_MEMBER_AT_HPP
