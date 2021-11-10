// Boost.Geometry

// Copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_MULTI_POINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_MULTI_POINT_HPP


#include <algorithm>
#include <vector>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/disjoint/box_box.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_box.hpp>
#include <boost/geometry/algorithms/detail/expand_by_epsilon.hpp>
#include <boost/geometry/algorithms/detail/within/point_in_geometry.hpp>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/policies/compare.hpp>

#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/disjoint.hpp>

#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace within {

struct multi_point_point
{
    template <typename MultiPoint, typename Point, typename Strategy>
    static inline bool apply(MultiPoint const& multi_point,
                             Point const& point,
                             Strategy const& strategy)
    {
        auto const s = strategy.relate(multi_point, point);

        typedef typename boost::range_const_iterator<MultiPoint>::type iterator;
        for ( iterator it = boost::begin(multi_point) ; it != boost::end(multi_point) ; ++it )
        {
            if (! s.apply(*it, point))
            {
                return false;
            }
        }

        // all points of MultiPoint inside Point
        return true;
    }
};

// NOTE: currently the strategy is ignored, math::equals() is used inside geometry::less<>
struct multi_point_multi_point
{
    template <typename MultiPoint1, typename MultiPoint2, typename Strategy>
    static inline bool apply(MultiPoint1 const& multi_point1,
                             MultiPoint2 const& multi_point2,
                             Strategy const& /*strategy*/)
    {
        typedef typename boost::range_value<MultiPoint2>::type point2_type;
        typedef typename Strategy::cs_tag cs_tag;
        typedef geometry::less<void, -1, cs_tag> less_type;

        less_type const less = less_type();

        std::vector<point2_type> points2(boost::begin(multi_point2), boost::end(multi_point2));
        std::sort(points2.begin(), points2.end(), less);

        bool result = false;

        typedef typename boost::range_const_iterator<MultiPoint1>::type iterator;
        for ( iterator it = boost::begin(multi_point1) ; it != boost::end(multi_point1) ; ++it )
        {
            if (! std::binary_search(points2.begin(), points2.end(), *it, less))
            {
                return false;
            }
            else
            {
                result = true;
            }
        }

        return result;
    }
};


// TODO: the complexity could be lesser
//   the second geometry could be "prepared"/sorted
// For Linear geometries partition could be used
// For Areal geometries point_in_geometry() would have to call the winding
//   strategy differently, currently it linearly calls the strategy for each
//   segment. So the segments would have to be sorted in a way consistent with
//   the strategy and then the strategy called only for the segments in range.
template <bool Within>
struct multi_point_single_geometry
{
    template <typename MultiPoint, typename LinearOrAreal, typename Strategy>
    static inline bool apply(MultiPoint const& multi_point,
                             LinearOrAreal const& linear_or_areal,
                             Strategy const& strategy)
    {
        //typedef typename boost::range_value<MultiPoint>::type point1_type;
        typedef typename point_type<LinearOrAreal>::type point2_type;
        typedef model::box<point2_type> box2_type;

        // Create envelope of geometry
        box2_type box;
        geometry::envelope(linear_or_areal, box, strategy);
        geometry::detail::expand_by_epsilon(box);

        // Test each Point with envelope and then geometry if needed
        // If in the exterior, break
        bool result = false;

        typedef typename boost::range_const_iterator<MultiPoint>::type iterator;
        for ( iterator it = boost::begin(multi_point) ; it != boost::end(multi_point) ; ++it )
        {
            typedef decltype(strategy.covered_by(*it, box)) point_in_box_type;

            int in_val = 0;
            
            // exterior of box and of geometry
            if (! point_in_box_type::apply(*it, box)
                || (in_val = point_in_geometry(*it, linear_or_areal, strategy)) < 0)
            {
                result = false;
                break;
            }

            // interior : interior/boundary
            if (Within ? in_val > 0 : in_val >= 0)
            {
                result = true;
            }
        }

        return result;
    }
};


// TODO: same here, probably the complexity could be lesser
template <bool Within>
struct multi_point_multi_geometry
{
    template <typename MultiPoint, typename LinearOrAreal, typename Strategy>
    static inline bool apply(MultiPoint const& multi_point,
                             LinearOrAreal const& linear_or_areal,
                             Strategy const& strategy)
    {
        typedef typename point_type<LinearOrAreal>::type point2_type;
        typedef model::box<point2_type> box2_type;
        static const bool is_linear = util::is_linear<LinearOrAreal>::value;

        // TODO: box pairs could be constructed on the fly, inside the rtree

        // Prepare range of envelopes and ids
        std::size_t count2 = boost::size(linear_or_areal);
        typedef std::pair<box2_type, std::size_t> box_pair_type;
        typedef std::vector<box_pair_type> box_pair_vector;
        box_pair_vector boxes(count2);
        for (std::size_t i = 0 ; i < count2 ; ++i)
        {
            geometry::envelope(linear_or_areal, boxes[i].first, strategy);
            geometry::detail::expand_by_epsilon(boxes[i].first);
            boxes[i].second = i;
        }

        // Create R-tree
        typedef index::parameters<index::rstar<4>, Strategy> index_parameters_type;
        index::rtree<box_pair_type, index_parameters_type>
            rtree(boxes.begin(), boxes.end(),
                  index_parameters_type(index::rstar<4>(), strategy));

        // For each point find overlapping envelopes and test corresponding single geometries
        // If a point is in the exterior break
        bool result = false;

        typedef typename boost::range_const_iterator<MultiPoint>::type iterator;
        for ( iterator it = boost::begin(multi_point) ; it != boost::end(multi_point) ; ++it )
        {
            // TODO: investigate the possibility of using satisfies
            // TODO: investigate the possibility of using iterative queries (optimization below)
            box_pair_vector inters_boxes;
            rtree.query(index::intersects(*it), std::back_inserter(inters_boxes));

            bool found_interior = false;
            bool found_boundary = false;
            int boundaries = 0;

            typedef typename box_pair_vector::const_iterator box_iterator;
            for (box_iterator box_it = inters_boxes.begin() ;
                 box_it != inters_boxes.end() ; ++box_it )
            {
                int const in_val = point_in_geometry(*it,
                    range::at(linear_or_areal, box_it->second), strategy);

                if (in_val > 0)
                {
                    found_interior = true;
                }
                else if (in_val == 0)
                {
                    ++boundaries;
                }

                // If the result was set previously (interior or
                // interior/boundary found) the only thing that needs to be
                // done for other points is to make sure they're not
                // overlapping the exterior no need to analyse boundaries.
                if (result && in_val >= 0)
                {
                    break;
                }
            }

            if (boundaries > 0)
            {
                if (is_linear && boundaries % 2 == 0)
                {
                    found_interior = true;
                }
                else
                {
                    found_boundary = true;
                }
            }

            // exterior
            if (! found_interior && ! found_boundary)
            {
                result = false;
                break;
            }

            // interior : interior/boundary
            if (Within ? found_interior : (found_interior || found_boundary))
            {
                result = true;
            }
        }

        return result;
    }
};

}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_MULTI_POINT_HPP
