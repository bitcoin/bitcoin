// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/symmetrical_base.hpp
/// \brief Base class for symmetrical types

#ifndef BOOST_BIMAP_RELATION_SYMMETRICAL_BASE_HPP
#define BOOST_BIMAP_RELATION_SYMMETRICAL_BASE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/mpl/if.hpp>
#include <boost/type_traits/remove_const.hpp>

// Boost.Bimap
#include <boost/bimap/tags/tagged.hpp>
#include <boost/bimap/tags/support/default_tagged.hpp>

#include <boost/bimap/relation/member_at.hpp>


namespace boost {
namespace bimaps {
namespace relation {

/// \brief Base of symmetrical tagged types.
/**

                                                           **/

template< class TA, class TB, bool force_mutable = false >
class symmetrical_base
{

    public:

    typedef BOOST_DEDUCED_TYPENAME tags::support::default_tagged
    <
        TA,
        member_at::left

    >::type tagged_left_type;

    typedef BOOST_DEDUCED_TYPENAME tags::support::default_tagged
    <
        TB,
        member_at::right

    >::type tagged_right_type;

    public:

    //@{
        /// The type stored in the relation

        typedef BOOST_DEDUCED_TYPENAME ::boost::mpl::if_c< force_mutable,

            BOOST_DEDUCED_TYPENAME ::boost::remove_const<
                BOOST_DEDUCED_TYPENAME tagged_left_type::value_type >::type,
            BOOST_DEDUCED_TYPENAME tagged_left_type::value_type

        >::type left_value_type;

        typedef BOOST_DEDUCED_TYPENAME ::boost::mpl::if_c< force_mutable,

            BOOST_DEDUCED_TYPENAME ::boost::remove_const<
                BOOST_DEDUCED_TYPENAME tagged_right_type::value_type >::type,
            BOOST_DEDUCED_TYPENAME tagged_right_type::value_type

        >::type right_value_type;
    //@}

    //@{
        /// The tag of the member. By default it is \c member_at::{side}
        typedef BOOST_DEDUCED_TYPENAME tagged_left_type ::tag  left_tag;
        typedef BOOST_DEDUCED_TYPENAME tagged_right_type::tag right_tag;
    //@}
};



} // namespace relation
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_RELATION_SYMMETRICAL_BASE_HPP

