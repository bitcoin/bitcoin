// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/manage_bimap_key.hpp
/// \brief Utility class to manage the set types of a bimap.

#ifndef BOOST_BIMAP_DETAIL_MANAGE_BIMAP_KEY_HPP
#define BOOST_BIMAP_DETAIL_MANAGE_BIMAP_KEY_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/bimap/detail/is_set_type_of.hpp>

#include <boost/bimap/set_of.hpp>

namespace boost {
namespace bimaps {
namespace detail {

/** \struct boost::bimaps::detail::manage_bimap_key
\brief Metafunction to manage the set types of a bimap.

\code
template< class Type >
struct manage_bimap_key
{
    typedef -SetType- type;
}
\endcode

See also bimap, bimap_core.
                                                                                **/

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Type >
struct manage_bimap_key
{

typedef BOOST_DEDUCED_TYPENAME

    mpl::eval_if< BOOST_DEDUCED_TYPENAME is_set_type_of< Type >::type,
    // {
            mpl::identity< Type >,
    // }
    // else
    // {
            // Default it to a set
            mpl::identity< set_of< Type > >
    // }

    >::type set_type;

    // Returns set_type and evaluate the concept_checked_type

    typedef BOOST_DEDUCED_TYPENAME mpl::if_c< true, set_type, 
        BOOST_DEDUCED_TYPENAME set_type::lazy_concept_checked::type
    >::type type;
};



#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

} // namespace detail
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_DETAIL_MANAGE_BIMAP_KEY_HPP


