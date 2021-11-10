// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_IMPLEMENTATION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_IMPLEMENTATION_HPP

// the implementation details
#include <boost/geometry/algorithms/detail/distance/point_to_geometry.hpp>
#include <boost/geometry/algorithms/detail/distance/multipoint_to_geometry.hpp>
#include <boost/geometry/algorithms/detail/distance/linear_to_linear.hpp>
#include <boost/geometry/algorithms/detail/distance/linear_or_areal_to_areal.hpp>
#include <boost/geometry/algorithms/detail/distance/linear_to_box.hpp>
#include <boost/geometry/algorithms/detail/distance/geometry_to_segment_or_box.hpp>
#include <boost/geometry/algorithms/detail/distance/segment_to_segment.hpp>
#include <boost/geometry/algorithms/detail/distance/segment_to_box.hpp>
#include <boost/geometry/algorithms/detail/distance/box_to_box.hpp>

#include <boost/geometry/strategies/distance/backward_compatibility.hpp>
#include <boost/geometry/strategies/distance/cartesian.hpp>
#include <boost/geometry/strategies/distance/geographic.hpp>
#include <boost/geometry/strategies/distance/spherical.hpp>

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_IMPLEMENTATION_HPP
