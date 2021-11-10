// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_VIEWS_REVERSIBLE_VIEW_HPP
#define BOOST_GEOMETRY_VIEWS_REVERSIBLE_VIEW_HPP


#include <boost/version.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/order_as_direction.hpp>

#include <boost/geometry/views/identity_view.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


// As template alias for now. It's possible that this should be a struct.
//   It'd also prevent instantiating the other, unneeded view.
template
<
    typename Range,
    order_selector Order = geometry::point_order<Range>::value
>
using clockwise_view = std::conditional_t
    <
        Order == counterclockwise,
        boost::reversed_range<Range>,
        identity_view<Range>
    >;


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief View on a range, reversing direction if necessary
\tparam Range original range
\tparam Direction direction of iteration
\ingroup views
*/
template <typename Range, iterate_direction Direction>
struct reversible_view {};


#ifndef DOXYGEN_NO_SPECIALIZATIONS

template <typename Range>
struct reversible_view<Range, iterate_forward>
{
    using type = identity_view<Range>;
};


template <typename Range>
struct reversible_view<Range, iterate_reverse>
{
    using type = boost::reversed_range<Range>;
};

#endif // DOXYGEN_NO_SPECIALIZATIONS


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_VIEWS_REVERSIBLE_VIEW_HPP
