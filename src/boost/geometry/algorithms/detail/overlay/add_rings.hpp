// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ADD_RINGS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ADD_RINGS_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>
#include <boost/throw_exception.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exception.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/detail/overlay/convert_ring.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_ring.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template
<
    typename GeometryOut,
    typename Geometry1,
    typename Geometry2,
    typename RingCollection
>
inline void convert_and_add(GeometryOut& result,
            Geometry1 const& geometry1, Geometry2 const& geometry2,
            RingCollection const& collection,
            ring_identifier id,
            bool reversed, bool append)
{
    typedef typename geometry::tag<Geometry1>::type tag1;
    typedef typename geometry::tag<Geometry2>::type tag2;
    typedef typename geometry::tag<GeometryOut>::type tag_out;

    if (id.source_index == 0)
    {
        convert_ring<tag_out>::apply(result,
                    get_ring<tag1>::apply(id, geometry1),
                    append, reversed);
    }
    else if (id.source_index == 1)
    {
        convert_ring<tag_out>::apply(result,
                    get_ring<tag2>::apply(id, geometry2),
                    append, reversed);
    }
    else if (id.source_index == 2)
    {
        convert_ring<tag_out>::apply(result,
                    get_ring<void>::apply(id, collection),
                    append, reversed);
    }
}

enum add_rings_error_handling
{
    add_rings_ignore_unordered,
    add_rings_add_unordered,
    add_rings_throw_if_reversed
};

template
<
    typename GeometryOut,
    typename SelectionMap,
    typename Geometry1,
    typename Geometry2,
    typename RingCollection,
    typename OutputIterator,
    typename Strategy
>
inline OutputIterator add_rings(SelectionMap const& map,
            Geometry1 const& geometry1, Geometry2 const& geometry2,
            RingCollection const& collection,
            OutputIterator out,
            Strategy const& strategy,
            add_rings_error_handling error_handling = add_rings_ignore_unordered)
{
    typedef typename SelectionMap::const_iterator iterator;

    std::size_t const min_num_points = core_detail::closure::minimum_ring_size
        <
            geometry::closure
                <
                    typename boost::range_value
                        <
                            RingCollection const
                        >::type
                >::value
        >::value;


    for (iterator it = boost::begin(map);
        it != boost::end(map);
        ++it)
    {
        if (! it->second.discarded
            && it->second.parent.source_index == -1)
        {
            GeometryOut result;
            convert_and_add(result, geometry1, geometry2, collection,
                    it->first, it->second.reversed, false);

            // Add children
            for (typename std::vector<ring_identifier>::const_iterator child_it
                        = it->second.children.begin();
                child_it != it->second.children.end();
                ++child_it)
            {
                iterator mit = map.find(*child_it);
                if (mit != map.end()
                    && ! mit->second.discarded)
                {
                    convert_and_add(result, geometry1, geometry2, collection,
                            *child_it, mit->second.reversed, true);
                }
            }

            // Only add rings if they satisfy minimal requirements.
            // This cannot be done earlier (during traversal), not
            // everything is figured out yet (sum of positive/negative rings)
            if (geometry::num_points(result) >= min_num_points)
            {
                typedef typename geometry::area_result<GeometryOut, Strategy>::type area_type;
                area_type const area = geometry::area(result, strategy);
                area_type const zero = 0;
                // Ignore if area is 0
                if (! math::equals(area, zero))
                {
                    if (error_handling == add_rings_add_unordered
                        || area > zero)
                    {
                        *out++ = result;
                    }
                    else if (error_handling == add_rings_throw_if_reversed)
                    {
                        BOOST_THROW_EXCEPTION(invalid_output_exception());
                    }
                }
            }
        }
    }
    return out;
}


template
<
    typename GeometryOut,
    typename SelectionMap,
    typename Geometry,
    typename RingCollection,
    typename OutputIterator,
    typename Strategy
>
inline OutputIterator add_rings(SelectionMap const& map,
            Geometry const& geometry,
            RingCollection const& collection,
            OutputIterator out,
            Strategy const& strategy)
{
    Geometry empty;
    return add_rings<GeometryOut>(map, geometry, empty, collection, out, strategy);
}


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ADD_RINGS_HPP
