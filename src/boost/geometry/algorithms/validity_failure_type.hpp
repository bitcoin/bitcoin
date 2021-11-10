// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_VALIDITY_FAILURE_TYPE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_VALIDITY_FAILURE_TYPE_HPP


namespace boost { namespace geometry
{


/*!
\brief Enumerates the possible validity failure types for a geometry
\ingroup enum
\details The enumeration validity_failure_type enumerates the possible
    reasons for which a geometry may be found as invalid by the
    is_valid algorithm.
    Besides the values that indicate invalidity, there is an
    additional value (no_failure) that indicates validity.

\qbk{
[heading See also]
[link geometry.reference.algorithms.is_valid The is_valid
algorithm taking a reference to validity_failure_type as second argument]
}
*/
enum validity_failure_type
{
    /// The geometry is valid
    ///
    no_failure = 0,
    /// The geometry has a very small number of points, e.g., less
    /// than 2 for linestrings, less than 3 for open rings, a closed
    /// multi-polygon that contains a polygon with less than 4 points, etc.
    /// (applies to linestrings, rings, polygons, multi-linestrings
    /// and multi-polygons)
    failure_few_points = 10,
    /// The topological dimension of the geometry is smaller than its
    /// dimension, e.g., a linestring with 3 identical points, an open
    /// polygon with an interior ring consisting of 3 collinear points, etc.
    /// (applies to linear and areal geometries, including segments
    /// and boxes)
    failure_wrong_topological_dimension = 11,
    /// The geometry contains spikes
    /// (applies to linear and areal geometries)
    failure_spikes = 12,
    /// The geometry has (consecutive) duplicate points
    /// (applies to areal geometries only)
    failure_duplicate_points = 13,
    /// The geometry is defined as closed, the starting/ending points
    /// are not equal
    /// (applies to areal geometries only)
    failure_not_closed = 20, // for areal geometries
    /// The geometry has invalid self-intersections.
    /// (applies to areal geometries only)
    failure_self_intersections = 21, // for areal geometries
    /// The actual orientation of the geometry is different from the one defined
    /// (applies to areal geometries only)
    failure_wrong_orientation = 22, // for areal geometries
    /// The geometry contains interior rings that lie outside the exterior ring
    /// (applies to polygons and multi-polygons only)
    failure_interior_rings_outside = 30, // for (multi-)polygons
    /// The geometry has nested interior rings
    /// (applies to polygons and multi-polygons only)
    failure_nested_interior_rings = 31, // for (multi-)polygons
    /// The interior of the geometry is disconnected
    /// (applies to polygons and multi-polygons only)
    failure_disconnected_interior = 32, // for (multi-)polygons
    /// The multi-polygon contains polygons whose interiors are not disjoint
    /// (applies to multi-polygons only)
    failure_intersecting_interiors = 40, // for multi-polygons
    /// The top-right corner of the box is lexicographically smaller
    /// than its bottom-left corner
    /// (applies to boxes only)
    failure_wrong_corner_order = 50, // for boxes
    /// The geometry has at least one point with an invalid coordinate
    /// (for example, the coordinate is a NaN)
    failure_invalid_coordinate = 60
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_VALIDITY_FAILURE_TYPE_HPP
