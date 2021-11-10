// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_CENTROID_TRANSLATING_TRANSFORMER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_CENTROID_TRANSLATING_TRANSFORMER_HPP


#include <cstddef>

#include <boost/core/addressof.hpp>
#include <boost/core/ref.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>

#include <boost/geometry/iterators/point_iterator.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace centroid
{


// NOTE: There is no need to translate in other coordinate systems than
// cartesian. But if it was needed then one should translate using
// CS-specific technique, e.g. in spherical/geographic a translation
// vector should contain coordinates being multiplies of 2PI or 360 deg.
template <typename Geometry,
          typename CastedTag = typename tag_cast
                                <
                                    typename tag<Geometry>::type,
                                    areal_tag
                                >::type,
    typename CSTag = typename cs_tag<Geometry>::type>
struct translating_transformer
{
    typedef typename geometry::point_type<Geometry>::type point_type;
    typedef boost::reference_wrapper<point_type const> result_type;

    explicit translating_transformer(Geometry const&) {}
    explicit translating_transformer(point_type const&) {}

    result_type apply(point_type const& pt) const
    {
        return result_type(pt);
    }

    template <typename ResPt>
    void apply_reverse(ResPt &) const {}
};

// Specialization for Areal Geometries in cartesian CS
template <typename Geometry>
struct translating_transformer<Geometry, areal_tag, cartesian_tag>
{
    typedef typename geometry::point_type<Geometry>::type point_type;
    typedef point_type result_type;
    
    explicit translating_transformer(Geometry const& geom)
        : m_origin(NULL)
    {
        geometry::point_iterator<Geometry const>
            pt_it = geometry::points_begin(geom);
        if ( pt_it != geometry::points_end(geom) )
        {
            m_origin = boost::addressof(*pt_it);
        }
    }

    explicit translating_transformer(point_type const& origin)
        : m_origin(boost::addressof(origin))
    {}

    result_type apply(point_type const& pt) const
    {
        point_type res = pt;
        if ( m_origin )
            geometry::subtract_point(res, *m_origin);
        return res;
    }

    template <typename ResPt>
    void apply_reverse(ResPt & res_pt) const
    {
        if ( m_origin )
            geometry::add_point(res_pt, *m_origin);
    }

    const point_type * m_origin;
};


}} // namespace detail::centroid
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_CENTROID_TRANSLATING_TRANSFORMER_HPP
