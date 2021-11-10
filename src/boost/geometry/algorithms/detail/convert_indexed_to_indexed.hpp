// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_CONVERT_INDEXED_TO_INDEXED_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_CONVERT_INDEXED_TO_INDEXED_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace conversion
{


template
<
    typename Source,
    typename Destination,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct indexed_to_indexed
{
    static inline void apply(Source const& source, Destination& destination)
    {
        typedef typename coordinate_type<Destination>::type coordinate_type;

        geometry::set<min_corner, Dimension>(destination,
            boost::numeric_cast<coordinate_type>(
                geometry::get<min_corner, Dimension>(source)));
        geometry::set<max_corner, Dimension>(destination,
            boost::numeric_cast<coordinate_type>(
                geometry::get<max_corner, Dimension>(source)));

        indexed_to_indexed
            <
                Source, Destination,
                Dimension + 1, DimensionCount
            >::apply(source, destination);
    }
};

template
<
    typename Source,
    typename Destination,
    std::size_t DimensionCount
>
struct indexed_to_indexed<Source, Destination, DimensionCount, DimensionCount>
{
    static inline void apply(Source const& , Destination& )
    {}
};


}} // namespace detail::conversion
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_CONVERT_INDEXED_TO_INDEXED_HPP
