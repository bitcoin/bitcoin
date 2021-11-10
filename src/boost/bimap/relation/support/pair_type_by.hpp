// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/support/pair_type_by.hpp
/// \brief pair_type_by<tag,relation> metafunction

#ifndef BOOST_BIMAP_RELATION_SUPPORT_PAIR_TYPE_BY_HPP
#define BOOST_BIMAP_RELATION_SUPPORT_PAIR_TYPE_BY_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/relation/detail/metadata_access_builder.hpp>

/** \struct boost::bimaps::relation::support::pair_type_by

\brief Metafunction to obtain the view type indexed by one of the sides.

\code

template< class Tag, class Relation >
struct pair_type_by
{
        typedef {signature-compatible with std::pair} type;
};

\endcode

See also member_at, pair_by().
\ingroup relation_group
                                                                    **/

namespace boost {
namespace bimaps {
namespace relation {
namespace support {

// Implementation of pair type by metafunction

BOOST_BIMAP_SYMMETRIC_METADATA_ACCESS_BUILDER
(
    pair_type_by,
    left_pair,
    right_pair
)

} // namespace support
} // namespace relation
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_RELATION_SUPPORT_PAIR_BY_TYPE_HPP

