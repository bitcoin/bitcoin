// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CONCEPTS_SIMPLIFY_CONCEPT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CONCEPTS_SIMPLIFY_CONCEPT_HPP

#include <iterator>
#include <type_traits>
#include <vector>

#include <boost/concept_check.hpp>
#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/concepts/distance_concept.hpp>


namespace boost { namespace geometry { namespace concepts
{


/*!
    \brief Checks strategy for simplify
    \ingroup simplify
*/
template <typename Strategy, typename Point>
struct SimplifyStrategy
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
private :

    // 1) must define distance_strategy_type,
    //    defining point-segment distance strategy (to be checked)
    typedef typename Strategy::distance_strategy_type ds_type;


    struct checker
    {
        template <typename ApplyMethod>
        static void apply(ApplyMethod)
        {
            namespace ft = boost::function_types;
            typedef typename ft::parameter_types
                <
                    ApplyMethod
                >::type parameter_types;

            typedef std::conditional_t
                <
                    ft::is_member_function_pointer<ApplyMethod>::value,
                    std::integral_constant<int, 1>,
                    std::integral_constant<int, 0>
                > base_index;

            BOOST_CONCEPT_ASSERT
                (
                    (concepts::PointSegmentDistanceStrategy<ds_type, Point, Point>)
                );

            Strategy *str = 0;
            std::vector<Point> const* v1 = 0;
            std::vector<Point> * v2 = 0;

            // 2) must implement method apply with arguments
            //    - Range
            //    - OutputIterator
            //    - floating point value
            str->apply(*v1, std::back_inserter(*v2), 1.0);

            boost::ignore_unused<parameter_types, base_index>();
            boost::ignore_unused(str);
        }
    };

public :
    BOOST_CONCEPT_USAGE(SimplifyStrategy)
    {
        checker::apply(&ds_type::template apply<Point, Point>);
    }
#endif
};



}}} // namespace boost::geometry::concepts

#endif // BOOST_GEOMETRY_STRATEGIES_CONCEPTS_SIMPLIFY_CONCEPT_HPP
