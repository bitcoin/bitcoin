// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/pair_layout.hpp
/// \brief Tags for pair layouts

#ifndef BOOST_BIMAP_RELATION_PAIR_LAYOUT_HPP
#define BOOST_BIMAP_RELATION_PAIR_LAYOUT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

namespace boost {
namespace bimaps {
namespace relation {

//@{

/// \brief Tag for normal layout. ( A,B -> A,B )

struct normal_layout {};

/// \brief Tag for mirror layout. ( A,B -> B,A )

struct mirror_layout {};

//@}

/** \struct boost::bimaps::relation::inverse_layout
\brief Metafunction to obtain the inverse of a layout.

\code
template< class Layout >
struct inverse_layout
{
    typedef {InverseLayout} type;
};
\endcode

See also normal_layout, mirror_layout.
                                                                    **/

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Layout >
struct inverse_layout
{
    typedef normal_layout type;
};

template<>
struct inverse_layout< normal_layout >
{
    typedef mirror_layout type;
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

} // namespace relation
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_RELATION_DETAIL_PAIR_LAYOUT_HPP

