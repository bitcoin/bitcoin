// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_RANGE_ADJACENT_FILTERED_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_RANGE_ADJACENT_FILTERED_HPP


#include <boost/range/adaptor/adjacent_filtered.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{

namespace traits
{

template<typename Filter, typename Geometry, bool DefaultPass>
#if BOOST_VERSION > 104500
struct tag<boost::adjacent_filtered_range<Filter, Geometry, DefaultPass> >
#else
struct tag<boost::range_detail::adjacent_filter_range<Filter, Geometry, DefaultPass> >
#endif
{
    typedef typename geometry::tag<Geometry>::type type;
};

}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_RANGE_ADJACENT_FILTERED_HPP

