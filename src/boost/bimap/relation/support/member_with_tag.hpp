// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/support/member_with_tag.hpp
/// \brief member_with_tag<tag,relation> metafunction

#ifndef BOOST_BIMAP_RELATION_SUPPORT_MEMBER_WITH_TAG_HPP
#define BOOST_BIMAP_RELATION_SUPPORT_MEMBER_WITH_TAG_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/relation/member_at.hpp>
#include <boost/bimap/detail/debug/static_error.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>

/** \struct boost::bimaps::relation::support::member_with_tag

\brief Metafunction to convert user tags to the member_at idiom.

\code

template< class Tag, class Relation >
struct member_with_tag
{
    typedef member_at::{side} type;
};

\endcode

We have to allow that all the metafunctions that works with tags
and retrieves data from a Relation will work with member_at idiom
even if the type was tagged. This will be great for the user,
because he can choose to tag a member after he is using the
relation and the code will still work.

If we perform this check in every metafunction it will be very
tedious and error prone, so instead of that all metafunctions
that works with relations first call this metafunction that
convert the tag to a member_at tag.

See also member_at, is_tag_of_member_at_left, is_tag_of_member_at_right.
\ingroup relation_group
                                                                    **/

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

namespace boost {
namespace bimaps {
namespace relation {
namespace support {

template
<
    class Tag,
    class Relation,
    class Enable = void
>
struct member_with_tag
{
    BOOST_BIMAP_STATIC_ERROR( MEMBER_WITH_TAG_FAILURE, (Relation,Tag) );
};

template< class Relation >
struct member_with_tag
<
    member_at::left, Relation, void
>
{
    typedef member_at::left type;
};

template< class Relation >
struct member_with_tag
<
    member_at::right, Relation, void
>
{
    typedef member_at::right type;
};

template< class Relation >
struct member_with_tag
<
    member_at::info, Relation, void
>
{
    typedef member_at::info type;
};


template< class Tag, class Relation >
struct member_with_tag
<
    Tag, Relation,
    BOOST_DEDUCED_TYPENAME enable_if
    <
        mpl::and_
        <
            mpl::not_< is_same<Tag,member_at::left> >,
            is_same
            <
                Tag,
                BOOST_DEDUCED_TYPENAME Relation::left_tag
            >
        >

    >::type
>
{
    typedef member_at::left type;
};

template< class Tag, class Relation >
struct member_with_tag
<
    Tag,
    Relation,
    BOOST_DEDUCED_TYPENAME enable_if
    <
        mpl::and_
        <
            mpl::not_< is_same<Tag,member_at::right> >,
            is_same
            <
                Tag,
                BOOST_DEDUCED_TYPENAME Relation::right_tag
            >
        >

    >::type
>
{
    typedef member_at::right type;
};

template< class Tag, class Relation >
struct member_with_tag
<
    Tag, Relation,
    BOOST_DEDUCED_TYPENAME enable_if
    <
        mpl::and_
        <
            mpl::not_< is_same<Tag,member_at::info> >,
            is_same
            <
                Tag,
                BOOST_DEDUCED_TYPENAME Relation::info_tag
            >
        >

    >::type
>
{
    typedef member_at::info type;
};

} // namespace support
} // namespace relation
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

#endif // BOOST_BIMAP_RELATION_SUPPORT_MEMBER_WITH_TAG_HPP


