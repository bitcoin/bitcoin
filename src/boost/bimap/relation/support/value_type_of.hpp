// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/support/value_type_of.hpp
/// \brief value_type_of<tag,relation> metafunction

#ifndef BOOST_BIMAP_RELATION_SUPPORT_VALUE_TYPE_OF_HPP
#define BOOST_BIMAP_RELATION_SUPPORT_VALUE_TYPE_OF_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/relation/detail/metadata_access_builder.hpp>

/** \struct boost::bimaps::relation::support::value_type_of

\brief Metafunction to obtain the value type of one of the sides.

\code

template< class Tag, class Relation >
struct value_type_of
{
        typedef typename Relation::{side}_type type;
};

\endcode

See also member_at, get().
\ingroup relation_group
                                                                    **/

namespace boost {
namespace bimaps {
namespace relation {
namespace support {

// Metafunction value_type_of
/*

    template< class Tag, class Relation >
    struct value_type_of
    {
        typedef -unspecified- type;
    };

*/

BOOST_BIMAP_SYMMETRIC_METADATA_ACCESS_BUILDER
(
    value_type_of,
    left_value_type,
    right_value_type
)

// Add support for info tags to value_type_of

template< class Tag, class SymmetricType >
struct value_type_of
<
    Tag, SymmetricType,
    BOOST_DEDUCED_TYPENAME enable_if
    <
        ::boost::bimaps::relation::support::is_tag_of_member_at_info
        <
            Tag,
            SymmetricType
        >

    >::type
>
{
    typedef BOOST_DEDUCED_TYPENAME SymmetricType::info_type type;
};

} // namespace support
} // namespace relation
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_RELATION_SUPPORT_VALUE_TYPE_OF_HPP

