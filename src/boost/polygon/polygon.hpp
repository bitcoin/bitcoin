/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_HPP
#define BOOST_POLYGON_POLYGON_HPP
#define BOOST_POLYGON_VERSION 014401

#include "isotropy.hpp"

//point
#include "point_data.hpp"
#include "point_traits.hpp"
#include "point_concept.hpp"

#include "transform.hpp"

//interval
#include "interval_data.hpp"
#include "interval_traits.hpp"
#include "interval_concept.hpp"

//rectangle
#include "rectangle_data.hpp"
#include "rectangle_traits.hpp"
#include "rectangle_concept.hpp"

//segment
#include "segment_data.hpp"
#include "segment_traits.hpp"
#include "segment_concept.hpp"

//algorithms needed by polygon types
#include "detail/iterator_points_to_compact.hpp"
#include "detail/iterator_compact_to_points.hpp"

//polygons
#include "polygon_45_data.hpp"
#include "polygon_data.hpp"
#include "polygon_90_data.hpp"
#include "polygon_90_with_holes_data.hpp"
#include "polygon_45_with_holes_data.hpp"
#include "polygon_with_holes_data.hpp"
#include "polygon_traits.hpp"

//manhattan boolean algorithms
#include "detail/boolean_op.hpp"
#include "detail/polygon_formation.hpp"
#include "detail/rectangle_formation.hpp"
#include "detail/max_cover.hpp"
#include "detail/property_merge.hpp"
#include "detail/polygon_90_touch.hpp"
#include "detail/iterator_geometry_to_set.hpp"

//45 boolean op algorithms
#include "detail/boolean_op_45.hpp"
#include "detail/polygon_45_formation.hpp"

//polygon set data types
#include "polygon_90_set_data.hpp"
//polygon set trait types
#include "polygon_90_set_traits.hpp"
//polygon set concepts
#include "polygon_90_set_concept.hpp"
//boolean operator syntax
#include "detail/polygon_90_set_view.hpp"

//45 boolean op algorithms
#include "detail/polygon_45_touch.hpp"
#include "detail/property_merge_45.hpp"
#include "polygon_45_set_data.hpp"
#include "polygon_45_set_traits.hpp"
#include "polygon_45_set_concept.hpp"
#include "detail/polygon_45_set_view.hpp"

//arbitrary polygon algorithms
#include "detail/polygon_arbitrary_formation.hpp"
#include "polygon_set_data.hpp"

//general scanline
#include "detail/scan_arbitrary.hpp"
#include "polygon_set_traits.hpp"
#include "detail/polygon_set_view.hpp"

#include "polygon_set_concept.hpp"

#include "segment_utils.hpp"

#endif
