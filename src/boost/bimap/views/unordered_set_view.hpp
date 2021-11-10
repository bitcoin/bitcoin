// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file views/unordered_set_view.hpp
/// \brief View of a bimap that is signature compatible with tr1::unordered_set.

#ifndef BOOST_BIMAP_VIEWS_UNORDERED_SET_VIEW_HPP
#define BOOST_BIMAP_VIEWS_UNORDERED_SET_VIEW_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/container_adaptor/unordered_set_adaptor.hpp>
#include <boost/bimap/detail/set_view_base.hpp>

namespace boost {
namespace bimaps {
namespace views {

/// \brief View of a bimap that is signature compatible with std::unordered_set.
/**

This class uses container_adaptor and iterator_adaptor to wrapped a index of the
multi_index bimap core so it can be used as a std::unordered_set.

See also const_unordered_set_view.
                                                                             **/

template< class CoreIndex >
class unordered_set_view
:
    public BOOST_BIMAP_SET_VIEW_CONTAINER_ADAPTOR(
        unordered_set_adaptor,
        CoreIndex,
        local_iterator,
        const_local_iterator
    ),

    public ::boost::bimaps::detail::
        set_view_base< unordered_set_view< CoreIndex >, CoreIndex >
{
    BOOST_BIMAP_SET_VIEW_BASE_FRIEND(unordered_set_view,CoreIndex)

    typedef BOOST_BIMAP_SET_VIEW_CONTAINER_ADAPTOR(
        unordered_set_adaptor,
        CoreIndex,
        local_iterator,
        const_local_iterator

    ) base_;

    public:

    unordered_set_view(BOOST_DEDUCED_TYPENAME base_::base_type & c)
        : base_(c) {}

    unordered_set_view & operator=(const unordered_set_view & v) 
    {
        this->base() = v.base();
        return *this;
    }
};


} // namespace views
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_VIEWS_UNORDERED_SET_VIEW_HPP

