// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_HOLES_PROXY_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_HOLES_PROXY_HPP

// Adapts Geometries from Boost.Polygon for usage in Boost.Geometry
// boost::polygon::polygon_with_holes_data -> boost::geometry::polygon
//   pair{begin_holes, begin_holes} -> interior_proxy

#include <boost/polygon/polygon.hpp>

#include <boost/geometry/geometries/adapted/boost_polygon/hole_iterator.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/ring_proxy.hpp>


namespace boost { namespace geometry
{

namespace adapt { namespace bp
{


// Polygon should implement the boost::polygon::polygon_with_holes_concept
// Specify constness in the template parameter if necessary
template<typename Polygon>
struct holes_proxy
{
    typedef ring_proxy
        <
            std::conditional_t
                <
                    std::is_const<Polygon>::value,
                    Polygon const,
                    Polygon
                >
        > proxy_type;
    typedef hole_iterator<Polygon, proxy_type> iterator_type;

    // The next line does not work probably because coordinate_type is part of the
    // polygon_traits, but not of the polygon_with_holes_traits
    // typedef typename boost::polygon::polygon_traits<Polygon>::coordinate_type coordinate_type;

    // So we use:
    typedef typename Polygon::coordinate_type coordinate_type;

    inline holes_proxy(Polygon& p)
        : polygon(p)
    {}

    inline void clear()
    {
        Polygon empty;
        // Clear the holes
        polygon.set_holes
            (
                boost::polygon::begin_holes(empty),
                boost::polygon::end_holes(empty)
            );
    }

    inline void resize(std::size_t new_size)
    {
        std::vector<boost::polygon::polygon_data<coordinate_type> > temporary_copy
            (
                boost::polygon::begin_holes(polygon),
                boost::polygon::end_holes(polygon)
            );
        temporary_copy.resize(new_size);
        polygon.set_holes(temporary_copy.begin(), temporary_copy.end());
    }

    template <typename Ring>
    inline void push_back(Ring const& ring)
    {
        std::vector<boost::polygon::polygon_data<coordinate_type> > temporary_copy
            (
                boost::polygon::begin_holes(polygon),
                boost::polygon::end_holes(polygon)
            );
        boost::polygon::polygon_data<coordinate_type> added;
        boost::polygon::set_points(added, ring.begin(), ring.end());
        temporary_copy.push_back(added);
        polygon.set_holes(temporary_copy.begin(), temporary_copy.end());
    }


    Polygon& polygon;
};


// Support holes_proxy for Boost.Range ADP

// Const versions
template<typename Polygon>
inline typename boost::geometry::adapt::bp::holes_proxy<Polygon const>::iterator_type
            range_begin(boost::geometry::adapt::bp::holes_proxy<Polygon const> const& proxy)
{
    typename boost::geometry::adapt::bp::holes_proxy<Polygon const>::iterator_type
            begin(proxy.polygon, boost::polygon::begin_holes(proxy.polygon));
    return begin;
}

template<typename Polygon>
inline typename boost::geometry::adapt::bp::holes_proxy<Polygon const>::iterator_type
            range_end(boost::geometry::adapt::bp::holes_proxy<Polygon const> const& proxy)
{
    typename boost::geometry::adapt::bp::holes_proxy<Polygon const>::iterator_type
            end(proxy.polygon, boost::polygon::end_holes(proxy.polygon));
    return end;
}

// Mutable versions
template<typename Polygon>
inline typename boost::geometry::adapt::bp::holes_proxy<Polygon>::iterator_type
            range_begin(boost::geometry::adapt::bp::holes_proxy<Polygon>& proxy)
{
    typename boost::geometry::adapt::bp::holes_proxy<Polygon>::iterator_type
            begin(proxy.polygon, boost::polygon::begin_holes(proxy.polygon));
    return begin;
}

template<typename Polygon>
inline typename boost::geometry::adapt::bp::holes_proxy<Polygon>::iterator_type
            range_end(boost::geometry::adapt::bp::holes_proxy<Polygon>& proxy)
{
    typename boost::geometry::adapt::bp::holes_proxy<Polygon>::iterator_type
            end(proxy.polygon, boost::polygon::end_holes(proxy.polygon));
    return end;
}


}}

namespace traits
{

template <typename Polygon>
struct rvalue_type<adapt::bp::holes_proxy<Polygon> >
{
    typedef adapt::bp::holes_proxy<Polygon> type;
};


template <typename Polygon>
struct clear<adapt::bp::holes_proxy<Polygon> >
{
    static inline void apply(adapt::bp::holes_proxy<Polygon> proxy)
    {
        proxy.clear();
    }
};

template <typename Polygon>
struct resize<adapt::bp::holes_proxy<Polygon> >
{
    static inline void apply(adapt::bp::holes_proxy<Polygon> proxy, std::size_t new_size)
    {
        proxy.resize(new_size);
    }
};

template <typename Polygon>
struct push_back<adapt::bp::holes_proxy<Polygon> >
{
    template <typename Ring>
    static inline void apply(adapt::bp::holes_proxy<Polygon> proxy, Ring const& ring)
    {
        proxy.push_back(ring);
    }
};



} // namespace traits


}}


// Specialize holes_proxy for Boost.Range
namespace boost
{
    template<typename Polygon>
    struct range_mutable_iterator<geometry::adapt::bp::holes_proxy<Polygon> >
    {
        typedef typename geometry::adapt::bp::holes_proxy<Polygon>::iterator_type type;
    };

    template<typename Polygon>
    struct range_const_iterator<geometry::adapt::bp::holes_proxy<Polygon> >
    {
        typedef typename geometry::adapt::bp::holes_proxy<Polygon const>::iterator_type type;
    };

} // namespace boost


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_HOLES_PROXY_HPP
