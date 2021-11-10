// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_INTERFACE_HPP

#include <boost/array.hpp>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/detail/as_range.hpp>
#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>
#include <boost/geometry/algorithms/detail/convex_hull/graham_andrew.hpp>
#include <boost/geometry/algorithms/is_empty.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/ring_type.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/convex_hull/services.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>

#include <boost/geometry/util/condition.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convex_hull
{

template <order_selector Order, closure_selector Closure>
struct hull_insert
{
    // Member template function (to avoid inconvenient declaration
    // of output-iterator-type, from hull_to_geometry)
    template <typename Geometry, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry const& geometry,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        typedef graham_andrew
            <
                Geometry,
                typename point_type<Geometry>::type
            > ConvexHullAlgorithm;

        ConvexHullAlgorithm algorithm;
        typename ConvexHullAlgorithm::state_type state;

        algorithm.apply(geometry, state, strategy);
        algorithm.result(state, out, Order == clockwise, Closure != open);

        return out;
    }
};

struct hull_to_geometry
{
    template <typename Geometry, typename OutputGeometry, typename Strategy>
    static inline void apply(Geometry const& geometry, OutputGeometry& out,
            Strategy const& strategy)
    {
        // TODO: Why not handle multi-polygon here?
        // TODO: detail::as_range() is only used in this place in the whole library
        //       it should probably be located here.
        // NOTE: A variable is created here because this can be a proxy range
        //       and back_insert_iterator<> can store a pointer to it.
        // Handle linestring, ring and polygon the same:
        auto&& range = detail::as_range(out);
        hull_insert
            <
                geometry::point_order<OutputGeometry>::value,
                geometry::closure<OutputGeometry>::value
            >::apply(geometry, range::back_inserter(range), strategy);
    }
};

}} // namespace detail::convex_hull
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct convex_hull
    : detail::convex_hull::hull_to_geometry
{};

// TODO: This is not correct in spherical and geographic CS
template <typename Box>
struct convex_hull<Box, box_tag>
{
    template <typename OutputGeometry, typename Strategy>
    static inline void apply(Box const& box,
                             OutputGeometry& out,
                             Strategy const& )
    {
        static bool const Close
            = geometry::closure<OutputGeometry>::value == closed;
        static bool const Reverse
            = geometry::point_order<OutputGeometry>::value == counterclockwise;

        // A hull for boxes is trivial. Any strategy is (currently) skipped.
        boost::array<typename point_type<Box>::type, 4> range;
        geometry::detail::assign_box_corners_oriented<Reverse>(box, range);
        geometry::append(out, range);
        if (BOOST_GEOMETRY_CONDITION(Close))
        {
            geometry::append(out, *boost::begin(range));
        }
    }
};



template <order_selector Order, closure_selector Closure>
struct convex_hull_insert
    : detail::convex_hull::hull_insert<Order, Closure>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy {

struct convex_hull
{
    template <typename Geometry, typename OutputGeometry, typename Strategy>
    static inline void apply(Geometry const& geometry,
                             OutputGeometry& out,
                             Strategy const& strategy)
    {
        //BOOST_CONCEPT_ASSERT( (geometry::concepts::ConvexHullStrategy<Strategy>) );
        dispatch::convex_hull<Geometry>::apply(geometry, out, strategy);
    }

    template <typename Geometry, typename OutputGeometry>
    static inline void apply(Geometry const& geometry,
                             OutputGeometry& out,
                             default_strategy)
    {
        typedef typename strategies::convex_hull::services::default_strategy
            <
                Geometry
            >::type strategy_type;

        apply(geometry, out, strategy_type());
    }
};

struct convex_hull_insert
{
    template <typename Geometry, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry const& geometry,
                                       OutputIterator& out,
                                       Strategy const& strategy)
    {
        //BOOST_CONCEPT_ASSERT( (geometry::concepts::ConvexHullStrategy<Strategy>) );

        return dispatch::convex_hull_insert<
                   geometry::point_order<Geometry>::value,
                   geometry::closure<Geometry>::value
               >::apply(geometry, out, strategy);
    }

    template <typename Geometry, typename OutputIterator>
    static inline OutputIterator apply(Geometry const& geometry,
                                       OutputIterator& out,
                                       default_strategy)
    {
        typedef typename strategies::convex_hull::services::default_strategy
            <
                Geometry
            >::type strategy_type;

        return apply(geometry, out, strategy_type());
    }
};

} // namespace resolve_strategy


namespace resolve_variant {

template <typename Geometry>
struct convex_hull
{
    template <typename OutputGeometry, typename Strategy>
    static inline void apply(Geometry const& geometry,
                             OutputGeometry& out,
                             Strategy const& strategy)
    {
        concepts::check_concepts_and_equal_dimensions<
            const Geometry,
            OutputGeometry
        >();

        resolve_strategy::convex_hull::apply(geometry, out, strategy);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct convex_hull<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename OutputGeometry, typename Strategy>
    struct visitor: boost::static_visitor<void>
    {
        OutputGeometry& m_out;
        Strategy const& m_strategy;

        visitor(OutputGeometry& out, Strategy const& strategy)
        : m_out(out), m_strategy(strategy)
        {}

        template <typename Geometry>
        void operator()(Geometry const& geometry) const
        {
            convex_hull<Geometry>::apply(geometry, m_out, m_strategy);
        }
    };

    template <typename OutputGeometry, typename Strategy>
    static inline void
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          OutputGeometry& out,
          Strategy const& strategy)
    {
        boost::apply_visitor(visitor<OutputGeometry, Strategy>(out, strategy),
                             geometry);
    }
};

template <typename Geometry>
struct convex_hull_insert
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry const& geometry,
                                       OutputIterator& out,
                                       Strategy const& strategy)
    {
        // Concept: output point type = point type of input geometry
        concepts::check<Geometry const>();
        concepts::check<typename point_type<Geometry>::type>();

        return resolve_strategy::convex_hull_insert::apply(geometry, out, strategy);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct convex_hull_insert<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename OutputIterator, typename Strategy>
    struct visitor: boost::static_visitor<OutputIterator>
    {
        OutputIterator& m_out;
        Strategy const& m_strategy;

        visitor(OutputIterator& out, Strategy const& strategy)
        : m_out(out), m_strategy(strategy)
        {}

        template <typename Geometry>
        OutputIterator operator()(Geometry const& geometry) const
        {
            return convex_hull_insert<Geometry>::apply(geometry, m_out, m_strategy);
        }
    };

    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          OutputIterator& out,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<OutputIterator, Strategy>(out, strategy), geometry);
    }
};

} // namespace resolve_variant


/*!
\brief \brief_calc{convex hull} \brief_strategy
\ingroup convex_hull
\details \details_calc{convex_hull,convex hull} \brief_strategy.
\tparam Geometry the input geometry type
\tparam OutputGeometry the output geometry type
\tparam Strategy the strategy type
\param geometry \param_geometry,  input geometry
\param out \param_geometry \param_set{convex hull}
\param strategy \param_strategy{area}

\qbk{distinguish,with strategy}

\qbk{[include reference/algorithms/convex_hull.qbk]}
 */
template<typename Geometry, typename OutputGeometry, typename Strategy>
inline void convex_hull(Geometry const& geometry,
            OutputGeometry& out, Strategy const& strategy)
{
    if (geometry::is_empty(geometry))
    {
        // Leave output empty
        return;
    }

    resolve_variant::convex_hull<Geometry>::apply(geometry, out, strategy);
}


/*!
\brief \brief_calc{convex hull}
\ingroup convex_hull
\details \details_calc{convex_hull,convex hull}.
\tparam Geometry the input geometry type
\tparam OutputGeometry the output geometry type
\param geometry \param_geometry,  input geometry
\param hull \param_geometry \param_set{convex hull}

\qbk{[include reference/algorithms/convex_hull.qbk]}
 */
template<typename Geometry, typename OutputGeometry>
inline void convex_hull(Geometry const& geometry,
            OutputGeometry& hull)
{
    geometry::convex_hull(geometry, hull, default_strategy());
}

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convex_hull
{


template<typename Geometry, typename OutputIterator, typename Strategy>
inline OutputIterator convex_hull_insert(Geometry const& geometry,
            OutputIterator out, Strategy const& strategy)
{
    return resolve_variant::convex_hull_insert
        <
            Geometry
        >::apply(geometry, out, strategy);
}


/*!
\brief Calculate the convex hull of a geometry, output-iterator version
\ingroup convex_hull
\tparam Geometry the input geometry type
\tparam OutputIterator: an output-iterator
\param geometry the geometry to calculate convex hull from
\param out an output iterator outputing points of the convex hull
\note This overloaded version outputs to an output iterator.
In this case, nothing is known about its point-type or
    about its clockwise order. Therefore, the input point-type
    and order are copied

 */
template<typename Geometry, typename OutputIterator>
inline OutputIterator convex_hull_insert(Geometry const& geometry,
            OutputIterator out)
{
    return convex_hull_insert(geometry, out, default_strategy());
}


}} // namespace detail::convex_hull
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_INTERFACE_HPP
