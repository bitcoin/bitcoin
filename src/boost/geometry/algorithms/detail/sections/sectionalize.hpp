// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2015 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTIONALIZE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTIONALIZE_HPP

#include <cstddef>
#include <type_traits>
#include <vector>

#include <boost/concept/requires.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/config.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/detail/expand_by_epsilon.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/detail/recalculate.hpp>
#include <boost/geometry/algorithms/detail/ring_identifier.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>
#include <boost/geometry/algorithms/detail/buffer/buffer_box.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>
#include <boost/geometry/policies/robustness/robust_point_type.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>
#include <boost/geometry/util/sequence.hpp>
#include <boost/geometry/views/detail/closed_clockwise_view.hpp>

// TEMP
#include <boost/geometry/strategy/envelope.hpp>
#include <boost/geometry/strategy/expand.hpp>

namespace boost { namespace geometry
{


/*!
    \brief Structure containing section information
    \details Section information consists of a bounding box, direction
        information (if it is increasing or decreasing, per dimension),
        index information (begin-end, ring, multi) and the number of
        segments in this section

    \tparam Box box-type
    \tparam DimensionCount number of dimensions for this section
    \ingroup sectionalize
 */
template
<
    typename Box,
    std::size_t DimensionCount
>
struct section
{
    typedef Box box_type;
    static std::size_t const dimension_count = DimensionCount;

    int directions[DimensionCount];
    ring_identifier ring_id;
    Box bounding_box;

    // NOTE: size_type could be passed as template parameter
    // NOTE: these probably also could be of type std::size_t
    signed_size_type begin_index;
    signed_size_type end_index;
    std::size_t count;
    std::size_t range_count;
    bool duplicate;
    signed_size_type non_duplicate_index;

    bool is_non_duplicate_first;
    bool is_non_duplicate_last;

    inline section()
        : begin_index(-1)
        , end_index(-1)
        , count(0)
        , range_count(0)
        , duplicate(false)
        , non_duplicate_index(-1)
        , is_non_duplicate_first(false)
        , is_non_duplicate_last(false)
    {
        assign_inverse(bounding_box);
        for (std::size_t i = 0; i < DimensionCount; i++)
        {
            directions[i] = 0;
        }
    }
};


/*!
    \brief Structure containing a collection of sections
    \note Derived from a vector, proves to be faster than of deque
    \note vector might be templated in the future
    \ingroup sectionalize
 */
template <typename Box, std::size_t DimensionCount>
struct sections : std::vector<section<Box, DimensionCount> >
{
    typedef Box box_type;
    static std::size_t const value = DimensionCount;
};


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace sectionalize
{

// NOTE: This utility will NOT work for latitudes, dimension 1 in spherical
// and geographic coordinate system because in these coordinate systems
// e.g. a segment on northern hemisphere may go towards greater latitude
// and then towards lesser latitude.
template
<
    typename Point,
    typename DimensionVector,
    std::size_t Index,
    std::size_t Count,
    typename CastedCSTag = typename tag_cast
                            <
                                typename cs_tag<Point>::type,
                                spherical_tag
                            >::type
>
struct get_direction_loop
{
    typedef typename util::sequence_element<Index, DimensionVector>::type dimension;

    template <typename Segment>
    static inline void apply(Segment const& seg,
                int directions[Count])
    {
        typedef typename coordinate_type<Segment>::type coordinate_type;

        coordinate_type const c0 = geometry::get<0, dimension::value>(seg);
        coordinate_type const c1 = geometry::get<1, dimension::value>(seg);

        directions[Index] = c1 > c0 ? 1 : c1 < c0 ? -1 : 0;

        get_direction_loop
        <
            Point,
            DimensionVector,
            Index + 1,
            Count,
            CastedCSTag
        >::apply(seg, directions);
    }
};

template
<
    typename Point,
    typename DimensionVector,
    std::size_t Count
>
struct get_direction_loop<Point, DimensionVector, 0, Count, spherical_tag>
{
    typedef typename util::sequence_element<0, DimensionVector>::type dimension;

    template <typename Segment>
    static inline void apply(Segment const& seg,
                int directions[Count])
    {
        typedef typename coordinate_type<Segment>::type coordinate_type;
        typedef typename coordinate_system<Point>::type::units units_t;

        coordinate_type const diff = math::longitude_distance_signed
                                        <
                                            units_t, coordinate_type
                                        >(geometry::get<0, 0>(seg),
                                          geometry::get<1, 0>(seg));

        coordinate_type zero = coordinate_type();
        directions[0] = diff > zero ? 1 : diff < zero ? -1 : 0;

        get_direction_loop
        <
            Point,
            DimensionVector,
            1,
            Count,
            spherical_tag
        >::apply(seg, directions);
    }
};

template
<
    typename Point,
    typename DimensionVector,
    std::size_t Count,
    typename CastedCSTag
>
struct get_direction_loop<Point, DimensionVector, Count, Count, CastedCSTag>
{
    template <typename Segment>
    static inline void apply(Segment const&, int [Count])
    {}
};


//! Copy one static array to another
template <typename T, std::size_t Index, std::size_t Count>
struct copy_loop
{
    static inline void apply(T const source[Count], T target[Count])
    {
        target[Index] = source[Index];
        copy_loop<T, Index + 1, Count>::apply(source, target);
    }
};

template <typename T, std::size_t Count>
struct copy_loop<T, Count, Count>
{
    static inline void apply(T const [Count], T [Count])
    {}
};

//! Compare two static arrays
template <typename T, std::size_t Index, std::size_t Count>
struct compare_loop
{
    static inline bool apply(T const array1[Count], T const array2[Count])
    {
        return array1[Index] != array2[Index]
            ? false
            : compare_loop
                <
                    T, Index + 1, Count
                >::apply(array1, array2);
    }
};

template <typename T, std::size_t Count>
struct compare_loop<T, Count, Count>
{
    static inline bool apply(T const [Count], T const [Count])
    {

        return true;
    }
};


template <std::size_t Dimension, std::size_t DimensionCount>
struct check_duplicate_loop
{
    template <typename Segment>
    static inline bool apply(Segment const& seg)
    {
        if (! geometry::math::equals
                (
                    geometry::get<0, Dimension>(seg),
                    geometry::get<1, Dimension>(seg)
                )
            )
        {
            return false;
        }

        return check_duplicate_loop
        <
                Dimension + 1, DimensionCount
        >::apply(seg);
    }
};

template <std::size_t DimensionCount>
struct check_duplicate_loop<DimensionCount, DimensionCount>
{
    template <typename Segment>
    static inline bool apply(Segment const&)
    {
        return true;
    }
};

//! Assign a value to a static array
template <typename T, std::size_t Index, std::size_t Count>
struct assign_loop
{
    static inline void apply(T dims[Count], T const value)
    {
        dims[Index] = value;
        assign_loop<T, Index + 1, Count>::apply(dims, value);
    }
};

template <typename T, std::size_t Count>
struct assign_loop<T, Count, Count>
{
    static inline void apply(T [Count], T const)
    {
    }
};

template <typename CSTag>
struct box_first_in_section
{
    template <typename Box, typename Point, typename Strategy>
    static inline void apply(Box & box, Point const& prev, Point const& curr,
                             Strategy const& strategy)
    {
        geometry::model::referring_segment<Point const> seg(prev, curr);
        geometry::envelope(seg, box, strategy);
    }
};

template <>
struct box_first_in_section<cartesian_tag>
{
    template <typename Box, typename Point, typename Strategy>
    static inline void apply(Box & box, Point const& prev, Point const& curr,
                             Strategy const& strategy)
    {
        geometry::envelope(prev, box, strategy);
        geometry::expand(box, curr, strategy);
    }
};

template <typename CSTag>
struct box_next_in_section
{
    template <typename Box, typename Point, typename Strategy>
    static inline void apply(Box & box, Point const& prev, Point const& curr,
                             Strategy const& strategy)
    {
        geometry::model::referring_segment<Point const> seg(prev, curr);
        geometry::expand(box, seg, strategy);
    }
};

template <>
struct box_next_in_section<cartesian_tag>
{
    template <typename Box, typename Point, typename Strategy>
    static inline void apply(Box & box, Point const& , Point const& curr,
                             Strategy const& strategy)
    {
        geometry::expand(box, curr, strategy);
    }
};

/// @brief Helper class to create sections of a part of a range, on the fly
template
<
    typename Point,
    typename DimensionVector
>
struct sectionalize_part
{
    static const std::size_t dimension_count
        = util::sequence_size<DimensionVector>::value;

    template
    <
        typename Iterator,
        typename RobustPolicy,
        typename Sections,
        typename Strategy
    >
    static inline void apply(Sections& sections,
                             Iterator begin, Iterator end,
                             RobustPolicy const& robust_policy,
                             Strategy const& strategy,
                             ring_identifier ring_id,
                             std::size_t max_count)
    {
        boost::ignore_unused(robust_policy);

        typedef typename boost::range_value<Sections>::type section_type;
        BOOST_STATIC_ASSERT
            (
                section_type::dimension_count
                 == util::sequence_size<DimensionVector>::value
            );

        typedef typename geometry::robust_point_type
            <
                Point,
                RobustPolicy
            >::type robust_point_type;

        std::size_t const count = std::distance(begin, end);
        if (count == 0)
        {
            return;
        }

        signed_size_type index = 0;
        signed_size_type ndi = 0; // non duplicate index
        section_type section;

        bool mark_first_non_duplicated = true;
        std::size_t last_non_duplicate_index = sections.size();

        Iterator it = begin;
        robust_point_type previous_robust_point;
        geometry::recalculate(previous_robust_point, *it, robust_policy);
        
        for(Iterator previous = it++;
            it != end;
            ++previous, ++it, index++)
        {
            robust_point_type current_robust_point;
            geometry::recalculate(current_robust_point, *it, robust_policy);
            model::referring_segment<robust_point_type> robust_segment(
                    previous_robust_point, current_robust_point);

            int direction_classes[dimension_count] = {0};
            get_direction_loop
            <
                Point, DimensionVector, 0, dimension_count
            >::apply(robust_segment, direction_classes);

            // if "dir" == 0 for all point-dimensions, it is duplicate.
            // Those sections might be omitted, if wished, lateron
            bool duplicate = false;

            if (direction_classes[0] == 0)
            {
                // Recheck because ALL dimensions should be checked,
                // not only first one.
                // (dimension_count might be < dimension<P>::value)
                if (check_duplicate_loop
                    <
                        0, geometry::dimension<Point>::type::value
                    >::apply(robust_segment)
                    )
                {
                    duplicate = true;

                    // Change direction-info to force new section
                    // Note that wo consecutive duplicate segments will generate
                    // only one duplicate-section.
                    // Actual value is not important as long as it is not -1,0,1
                    assign_loop
                    <
                        int, 0, dimension_count
                    >::apply(direction_classes, -99);
                }
            }

            if (section.count > 0
                && (! compare_loop
                        <
                            int, 0, dimension_count
                        >::apply(direction_classes, section.directions)
                    || section.count > max_count)
                )
            {
                if (! section.duplicate)
                {
                    last_non_duplicate_index = sections.size();
                }

                sections.push_back(section);
                section = section_type();
            }

            if (section.count == 0)
            {
                section.begin_index = index;
                section.ring_id = ring_id;
                section.duplicate = duplicate;
                section.non_duplicate_index = ndi;
                section.range_count = count;

                if (mark_first_non_duplicated && ! duplicate)
                {
                    section.is_non_duplicate_first = true;
                    mark_first_non_duplicated = false;
                }

                copy_loop
                    <
                        int, 0, dimension_count
                    >::apply(direction_classes, section.directions);

                // In cartesian this is envelope of previous point expanded with current point
                // in non-cartesian this is envelope of a segment
                box_first_in_section<typename cs_tag<robust_point_type>::type>
                    ::apply(section.bounding_box, previous_robust_point, current_robust_point, strategy);
            }
            else
            {
                // In cartesian this is expand with current point
                // in non-cartesian this is expand with a segment
                box_next_in_section<typename cs_tag<robust_point_type>::type>
                    ::apply(section.bounding_box, previous_robust_point, current_robust_point, strategy);
            }

            section.end_index = index + 1;
            section.count++;
            if (! duplicate)
            {
                ndi++;
            }
            previous_robust_point = current_robust_point;
        }

        // Add last section if applicable
        if (section.count > 0)
        {
            if (! section.duplicate)
            {
                last_non_duplicate_index = sections.size();
            }

            sections.push_back(section);
        }

        if (last_non_duplicate_index < sections.size()
            && ! sections[last_non_duplicate_index].duplicate)
        {
            sections[last_non_duplicate_index].is_non_duplicate_last = true;
        }
    }
};


template
<
    closure_selector Closure,
    bool Reverse,
    typename Point,
    typename DimensionVector
>
struct sectionalize_range
{
    template
    <
        typename Range,
        typename RobustPolicy,
        typename Sections,
        typename Strategy
    >
    static inline void apply(Range const& range,
                             RobustPolicy const& robust_policy,
                             Sections& sections,
                             Strategy const& strategy,
                             ring_identifier ring_id,
                             std::size_t max_count)
    {
        detail::closed_clockwise_view
            <
                Range const,
                Closure,
                Reverse ? counterclockwise : clockwise
            > const view(range);

        std::size_t const n = boost::size(view);
        if (n == 0)
        {
            // Zero points, no section
            return;
        }

        if (n == 1)
        {
            // Line with one point ==> no sections
            return;
        }

        sectionalize_part<Point, DimensionVector>::apply(sections,
            boost::begin(view), boost::end(view),
            robust_policy, strategy,
            ring_id, max_count);
    }
};

template
<
    bool Reverse,
    typename DimensionVector
>
struct sectionalize_polygon
{
    template
    <
        typename Polygon,
        typename RobustPolicy,
        typename Sections,
        typename Strategy
    >
    static inline void apply(Polygon const& poly,
                RobustPolicy const& robust_policy,
                Sections& sections,
                Strategy const& strategy,
                ring_identifier ring_id,
                std::size_t max_count)
    {
        typedef typename point_type<Polygon>::type point_type;
        typedef sectionalize_range
            <
                closure<Polygon>::value, Reverse, point_type, DimensionVector
            > per_range;

        ring_id.ring_index = -1;
        per_range::apply(exterior_ring(poly), robust_policy, sections,
                         strategy, ring_id, max_count);

        ring_id.ring_index++;
        typename interior_return_type<Polygon const>::type
            rings = interior_rings(poly);
        for (typename detail::interior_iterator<Polygon const>::type
                it = boost::begin(rings); it != boost::end(rings); ++it, ++ring_id.ring_index)
        {
            per_range::apply(*it, robust_policy, sections,
                             strategy, ring_id, max_count);
        }
    }
};

template <typename DimensionVector>
struct sectionalize_box
{
    template
    <
        typename Box,
        typename RobustPolicy,
        typename Sections,
        typename Strategy
    >
    static inline void apply(Box const& box,
                RobustPolicy const& robust_policy,
                Sections& sections,
                Strategy const& strategy,
                ring_identifier const& ring_id, std::size_t max_count)
    {
        typedef typename point_type<Box>::type point_type;

        assert_dimension<Box, 2>();

        // Add all four sides of the 2D-box as separate section.
        // Easiest is to convert it to a polygon.
        // However, we don't have the polygon type
        // (or polygon would be a helper-type).
        // Therefore we mimic a linestring/std::vector of 5 points

        // TODO: might be replaced by assign_box_corners_oriented
        // or just "convert"
        point_type ll, lr, ul, ur;
        geometry::detail::assign_box_corners(box, ll, lr, ul, ur);

        std::vector<point_type> points;
        points.push_back(ll);
        points.push_back(ul);
        points.push_back(ur);
        points.push_back(lr);
        points.push_back(ll);

        // NOTE: Use cartesian envelope strategy in all coordinate systems
        //       because edges of a box are not geodesic segments
        sectionalize_range
            <
                closed, false, point_type, DimensionVector
            >::apply(points, robust_policy, sections,
                     strategy, ring_id, max_count);
    }
};

template <typename DimensionVector, typename Policy>
struct sectionalize_multi
{
    template
    <
        typename MultiGeometry,
        typename RobustPolicy,
        typename Sections,
        typename Strategy
    >
    static inline void apply(MultiGeometry const& multi,
                RobustPolicy const& robust_policy,
                Sections& sections,
                Strategy const& strategy,
                ring_identifier ring_id,
                std::size_t max_count)
    {
        ring_id.multi_index = 0;
        for (typename boost::range_iterator<MultiGeometry const>::type
                    it = boost::begin(multi);
            it != boost::end(multi);
            ++it, ++ring_id.multi_index)
        {
            Policy::apply(*it, robust_policy, sections,
                          strategy,
                          ring_id, max_count);
        }
    }
};

// TODO: If it depends on CS it should probably be made into strategy.
// For now implemented here because of ongoing work on robustness
//   the fact that it interferes with detail::buffer::buffer_box
//   and that we probably need a general strategy for defining epsilon in
//   various coordinate systems, e.g. for point comparison, enlargement of
//   bounding boxes, etc.
template <typename CSTag>
struct expand_by_epsilon
    : not_implemented<CSTag>
{};

template <>
struct expand_by_epsilon<cartesian_tag>
{
    template <typename Box>
    static inline void apply(Box & box)
    {
        detail::expand_by_epsilon(box);
    }
};

template <>
struct expand_by_epsilon<spherical_tag>
{
    template <typename Box>
    static inline void apply(Box & box)
    {
        typedef typename coordinate_type<Box>::type coord_t;
        static const coord_t eps = std::is_same<coord_t, float>::value
            ? coord_t(1e-6)
            : coord_t(1e-12);
        detail::expand_by_epsilon(box, eps);
    }
};

// TODO: In geographic CS it should probably also depend on FormulaPolicy.
template <>
struct expand_by_epsilon<geographic_tag>
    : expand_by_epsilon<spherical_tag>
{};

template <typename Sections, typename Strategy>
inline void enlarge_sections(Sections& sections, Strategy const&)
{
    // Enlarge sections slightly, this should be consistent with math::equals()
    // and with the tolerances used in general_form intersections.
    // This avoids missing turns.
    
    // Points and Segments are equal-compared WRT machine epsilon, but Boxes aren't
    // Enlarging Boxes ensures that they correspond to the bound objects,
    // Segments in this case, since Sections are collections of Segments.

    // It makes section a tiny bit too large, which might cause (a small number)
    // of more comparisons
    for (typename boost::range_iterator<Sections>::type it = boost::begin(sections);
        it != boost::end(sections);
        ++it)
    {
#if defined(BOOST_GEOMETRY_USE_RESCALING)
        detail::sectionalize::expand_by_epsilon
            <
                typename Strategy::cs_tag
            >::apply(it->bounding_box);

#else
        // Expand the box to avoid missing any intersection. The amount is
        // should be larger than epsilon. About the value itself: the smaller
        // it is, the higher the risk to miss intersections. The larger it is,
        // the more comparisons are made. So it should be on the high side.
        detail::buffer::buffer_box(it->bounding_box, 0.001, it->bounding_box);
#endif
    }
}


}} // namespace detail::sectionalize
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename Geometry,
    bool Reverse,
    typename DimensionVector
>
struct sectionalize
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not or not yet implemented for this Geometry type.",
        Tag, Geometry);
};

template
<
    typename Box,
    bool Reverse,
    typename DimensionVector
>
struct sectionalize<box_tag, Box, Reverse, DimensionVector>
    : detail::sectionalize::sectionalize_box<DimensionVector>
{};

template
<
    typename LineString,
    typename DimensionVector
>
struct sectionalize
    <
        linestring_tag,
        LineString,
        false,
        DimensionVector
    >
    : detail::sectionalize::sectionalize_range
        <
            closed, false,
            typename point_type<LineString>::type,
            DimensionVector
        >
{};

template
<
    typename Ring,
    bool Reverse,
    typename DimensionVector
>
struct sectionalize<ring_tag, Ring, Reverse, DimensionVector>
    : detail::sectionalize::sectionalize_range
        <
            geometry::closure<Ring>::value, Reverse,
            typename point_type<Ring>::type,
            DimensionVector
        >
{};

template
<
    typename Polygon,
    bool Reverse,
    typename DimensionVector
>
struct sectionalize<polygon_tag, Polygon, Reverse, DimensionVector>
    : detail::sectionalize::sectionalize_polygon
        <
            Reverse, DimensionVector
        >
{};

template
<
    typename MultiPolygon,
    bool Reverse,
    typename DimensionVector
>
struct sectionalize<multi_polygon_tag, MultiPolygon, Reverse, DimensionVector>
    : detail::sectionalize::sectionalize_multi
        <
            DimensionVector,
            detail::sectionalize::sectionalize_polygon
                <
                    Reverse,
                    DimensionVector
                >
        >

{};

template
<
    typename MultiLinestring,
    bool Reverse,
    typename DimensionVector
>
struct sectionalize<multi_linestring_tag, MultiLinestring, Reverse, DimensionVector>
    : detail::sectionalize::sectionalize_multi
        <
            DimensionVector,
            detail::sectionalize::sectionalize_range
                <
                    closed, false,
                    typename point_type<MultiLinestring>::type,
                    DimensionVector
                >
        >

{};

} // namespace dispatch
#endif


/*!
    \brief Split a geometry into monotonic sections
    \ingroup sectionalize
    \tparam Geometry type of geometry to check
    \tparam Sections type of sections to create
    \param geometry geometry to create sections from
    \param robust_policy policy to handle robustness issues
    \param sections structure with sections
    \param strategy strategy for envelope calculation
    \param expand_strategy strategy for partitions
    \param source_index index to assign to the ring_identifiers
    \param max_count maximal number of points per section
        (defaults to 10, this seems to give the fastest results)

 */
template
<
    bool Reverse,
    typename DimensionVector,
    typename Geometry,
    typename Sections,
    typename RobustPolicy,
    typename Strategy
>
inline void sectionalize(Geometry const& geometry,
                RobustPolicy const& robust_policy,
                Sections& sections,
                Strategy const& strategy,
                int source_index = 0,
                std::size_t max_count = 10)
{
    concepts::check<Geometry const>();

    typedef typename boost::range_value<Sections>::type section_type;

    // Compiletime check for point type of section boxes
    // and point type related to robust policy
    typedef typename geometry::coordinate_type
    <
        typename section_type::box_type
    >::type ctype1;
    typedef typename geometry::coordinate_type
    <
        typename geometry::robust_point_type
        <
            typename geometry::point_type<Geometry>::type,
            RobustPolicy
        >::type
    >::type ctype2;

    BOOST_STATIC_ASSERT((std::is_same<ctype1, ctype2>::value));


    sections.clear();

    ring_identifier ring_id;
    ring_id.source_index = source_index;

    dispatch::sectionalize
        <
            typename tag<Geometry>::type,
            Geometry,
            Reverse,
            DimensionVector
        >::apply(geometry, robust_policy, sections,
                 strategy,
                 ring_id, max_count);

    detail::sectionalize::enlarge_sections(sections, strategy);
}


template
<
    bool Reverse,
    typename DimensionVector,
    typename Geometry,
    typename Sections,
    typename RobustPolicy
>
inline void sectionalize(Geometry const& geometry,
                         RobustPolicy const& robust_policy,
                         Sections& sections,
                         int source_index = 0,
                         std::size_t max_count = 10)
{
    typedef typename strategies::envelope::services::default_strategy
        <
            Geometry,
            model::box<typename point_type<Geometry>::type>
        >::type strategy_type;

    boost::geometry::sectionalize
        <
            Reverse, DimensionVector
        >(geometry, robust_policy, sections,
          strategy_type(),
          source_index, max_count);
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTIONALIZE_HPP
