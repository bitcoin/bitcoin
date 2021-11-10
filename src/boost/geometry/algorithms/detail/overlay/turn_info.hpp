// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TURN_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TURN_INFO_HPP


#include <boost/array.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/policies/robustness/segment_ratio.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

enum method_type
{
    method_none,
    method_disjoint,
    method_crosses,
    method_touch,
    method_touch_interior,
    method_collinear,
    method_equal,
    method_start,
    method_error
};


/*!
    \brief Turn operation: operation
    \details Information necessary for traversal phase (a phase
        of the overlay process). The information is gathered during the
        get_turns (segment intersection) phase.
        The class is to be included in the turn_info class, either direct
        or a derived or similar class with more (e.g. enrichment) information.
 */
template <typename Point, typename SegmentRatio>
struct turn_operation
{
    typedef SegmentRatio segment_ratio_type;

    operation_type operation;
    segment_identifier seg_id;
    SegmentRatio fraction;

    typedef typename coordinate_type<Point>::type comparable_distance_type;
    comparable_distance_type remaining_distance;

    inline turn_operation()
        : operation(operation_none)
        , remaining_distance(0)
    {}
};


/*!
    \brief Turn information: intersection point, method, and turn information
    \details Information necessary for traversal phase (a phase
        of the overlay process). The information is gathered during the
        get_turns (segment intersection) phase.
    \tparam Point point type of intersection point
    \tparam Operation gives classes opportunity to add additional info
    \tparam Container gives classes opportunity to define how operations are stored
 */
template
<
    typename Point,
    typename SegmentRatio = geometry::segment_ratio<typename coordinate_type<Point>::type>,
    typename Operation = turn_operation<Point, SegmentRatio>,
    typename Container = boost::array<Operation, 2>
>
struct turn_info
{
    typedef Point point_type;
    typedef SegmentRatio segment_ratio_type;
    typedef Operation turn_operation_type;
    typedef Container container_type;

    Point point;
    method_type method;
    bool touch_only; // True in case of method touch(interior) and lines do not cross
    signed_size_type cluster_id; // For multiple turns on same location, > 0. Else -1. 0 is unused.
    bool discarded;
    bool has_colocated_both; // Colocated with a uu turn (for union) or ii (other)

    Container operations;

    inline turn_info()
        : method(method_none)
        , touch_only(false)
        , cluster_id(-1)
        , discarded(false)
        , has_colocated_both(false)
    {}

    inline bool both(operation_type type) const
    {
        return has12(type, type);
    }

    inline bool has(operation_type type) const
    {
        return this->operations[0].operation == type
            || this->operations[1].operation == type;
    }

    inline bool combination(operation_type type1, operation_type type2) const
    {
        return has12(type1, type2) || has12(type2, type1);
    }

    inline bool blocked() const
    {
        return both(operation_blocked);
    }
    inline bool opposite() const
    {
        return both(operation_opposite);
    }
    inline bool any_blocked() const
    {
        return has(operation_blocked);
    }
    inline bool is_clustered() const
    {
        return cluster_id > 0;
    }

private :
    inline bool has12(operation_type type1, operation_type type2) const
    {
        return this->operations[0].operation == type1
            && this->operations[1].operation == type2
            ;
    }

};


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TURN_INFO_HPP
