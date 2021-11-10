// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2017-2021.
// Modifications copyright (c) 2017-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_AREA_HPP
#define BOOST_GEOMETRY_ALGORITHMS_AREA_HPP

#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>

#include <boost/geometry/algorithms/detail/calculate_null.hpp>
#include <boost/geometry/algorithms/detail/calculate_sum.hpp>
// #include <boost/geometry/algorithms/detail/throw_on_empty_input.hpp>
#include <boost/geometry/algorithms/detail/multi_sum.hpp>
#include <boost/geometry/algorithms/detail/visit.hpp>

#include <boost/geometry/algorithms/area_result.hpp>
#include <boost/geometry/algorithms/default_area_result.hpp>

#include <boost/geometry/geometries/adapted/boost_variant.hpp> // For backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/area/services.hpp>
#include <boost/geometry/strategies/area/cartesian.hpp>
#include <boost/geometry/strategies/area/geographic.hpp>
#include <boost/geometry/strategies/area/spherical.hpp>
#include <boost/geometry/strategies/concepts/area_concept.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/views/detail/closed_clockwise_view.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace area
{

struct box_area
{
    template <typename Box, typename Strategies>
    static inline typename coordinate_type<Box>::type
    apply(Box const& box, Strategies const& strategies)
    {
        // Currently only works for 2D Cartesian boxes
        assert_dimension<Box, 2>();

        return strategies.area(box).apply(box);
    }
};


struct ring_area
{
    template <typename Ring, typename Strategies>
    static inline typename area_result<Ring, Strategies>::type
    apply(Ring const& ring, Strategies const& strategies)
    {
        using strategy_type = decltype(strategies.area(ring));

        BOOST_CONCEPT_ASSERT( (geometry::concepts::AreaStrategy<Ring, strategy_type>) );
        assert_dimension<Ring, 2>();

        // Ignore warning (because using static method sometimes) on strategy
        boost::ignore_unused(strategies);

        // An open ring has at least three points,
        // A closed ring has at least four points,
        // if not, there is no (zero) area
        if (boost::size(ring) < detail::minimum_ring_size<Ring>::value)
        {
            return typename area_result<Ring, Strategies>::type();
        }

        detail::closed_clockwise_view<Ring const> const view(ring);
        auto it = boost::begin(view);
        auto const end = boost::end(view);

        strategy_type const strategy = strategies.area(ring);
        typename strategy_type::template state<Ring> state;        

        for (auto previous = it++; it != end; ++previous, ++it)
        {
            strategy.apply(*previous, *it, state);
        }

        return strategy.result(state);
    }
};


}} // namespace detail::area


#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct area : detail::calculate_null
{
    template <typename Strategy>
    static inline typename area_result<Geometry, Strategy>::type
        apply(Geometry const& geometry, Strategy const& strategy)
    {
        return calculate_null::apply
            <
                typename area_result<Geometry, Strategy>::type
            >(geometry, strategy);
    }
};


template <typename Geometry>
struct area<Geometry, box_tag> : detail::area::box_area
{};


template <typename Ring>
struct area<Ring, ring_tag>
    : detail::area::ring_area
{};


template <typename Polygon>
struct area<Polygon, polygon_tag> : detail::calculate_polygon_sum
{
    template <typename Strategy>
    static inline typename area_result<Polygon, Strategy>::type
        apply(Polygon const& polygon, Strategy const& strategy)
    {
        return calculate_polygon_sum::apply
            <
                typename area_result<Polygon, Strategy>::type,
                detail::area::ring_area
            >(polygon, strategy);
    }
};


template <typename MultiGeometry>
struct area<MultiGeometry, multi_polygon_tag> : detail::multi_sum
{
    template <typename Strategy>
    static inline typename area_result<MultiGeometry, Strategy>::type
    apply(MultiGeometry const& multi, Strategy const& strategy)
    {
        return multi_sum::apply
               <
                   typename area_result<MultiGeometry, Strategy>::type,
                   area<typename boost::range_value<MultiGeometry>::type>
               >(multi, strategy);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy
{

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct area
{
    template <typename Geometry>
    static inline typename area_result<Geometry, Strategy>::type
    apply(Geometry const& geometry, Strategy const& strategy)
    {
        return dispatch::area<Geometry>::apply(geometry, strategy);
    }
};

template <typename Strategy>
struct area<Strategy, false>
{
    template <typename Geometry>
    static auto apply(Geometry const& geometry, Strategy const& strategy)
    {
        using strategies::area::services::strategy_converter;
        return dispatch::area
            <
                Geometry
            >::apply(geometry, strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct area<default_strategy, false>
{
    template <typename Geometry>
    static inline typename area_result<Geometry>::type
    apply(Geometry const& geometry, default_strategy)
    {
        typedef typename strategies::area::services::default_strategy
            <
                Geometry
            >::type strategy_type;

        return dispatch::area<Geometry>::apply(geometry, strategy_type());
    }
};


} // namespace resolve_strategy


namespace resolve_dynamic
{

template <typename Geometry, typename Tag = typename geometry::tag<Geometry>::type>
struct area
{
    template <typename Strategy>
    static inline typename area_result<Geometry, Strategy>::type
        apply(Geometry const& geometry, Strategy const& strategy)
    {
        return resolve_strategy::area<Strategy>::apply(geometry, strategy);
    }
};

template <typename Geometry>
struct area<Geometry, dynamic_geometry_tag>
{
    template <typename Strategy>
    static inline typename area_result<Geometry, Strategy>::type
        apply(Geometry const& geometry, Strategy const& strategy)
    {
        typename area_result<Geometry, Strategy>::type result = 0;
        traits::visit<Geometry>::apply([&](auto const& g)
        {
            result = area<util::remove_cref_t<decltype(g)>>::apply(g, strategy);
        }, geometry);
        return result;
    }
};

template <typename Geometry>
struct area<Geometry, geometry_collection_tag>
{
    template <typename Strategy>
    static inline typename area_result<Geometry, Strategy>::type
        apply(Geometry const& geometry, Strategy const& strategy)
    {
        typename area_result<Geometry, Strategy>::type result = 0;
        detail::visit_breadth_first([&](auto const& g)
        {
            result += area<util::remove_cref_t<decltype(g)>>::apply(g, strategy);
            return true;
        }, geometry);
        return result;
    }
};

} // namespace resolve_dynamic


/*!
\brief \brief_calc{area}
\ingroup area
\details \details_calc{area}. \details_default_strategy

The area algorithm calculates the surface area of all geometries having a surface, namely
box, polygon, ring, multipolygon. The units are the square of the units used for the points
defining the surface. If subject geometry is defined in meters, then area is calculated
in square meters.

The area calculation can be done in all three common coordinate systems, Cartesian, Spherical
and Geographic as well.

\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{area}

\qbk{[include reference/algorithms/area.qbk]}
\qbk{[heading Examples]}
\qbk{[area] [area_output]}
*/
template <typename Geometry>
inline typename area_result<Geometry>::type
area(Geometry const& geometry)
{
    concepts::check<Geometry const>();

    // detail::throw_on_empty_input(geometry);

    return resolve_dynamic::area<Geometry>::apply(geometry, default_strategy());
}

/*!
\brief \brief_calc{area} \brief_strategy
\ingroup area
\details \details_calc{area} \brief_strategy. \details_strategy_reasons
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{Area}
\param geometry \param_geometry
\param strategy \param_strategy{area}
\return \return_calc{area}

\qbk{distinguish,with strategy}

\qbk{
[include reference/algorithms/area.qbk]

[heading Available Strategies]
\* [link geometry.reference.strategies.strategy_area_cartesian Cartesian]
\* [link geometry.reference.strategies.strategy_area_spherical Spherical]
\* [link geometry.reference.strategies.strategy_area_geographic Geographic]

[heading Example]
[area_with_strategy]
[area_with_strategy_output]
}
 */
template <typename Geometry, typename Strategy>
inline typename area_result<Geometry, Strategy>::type
area(Geometry const& geometry, Strategy const& strategy)
{
    concepts::check<Geometry const>();

    // detail::throw_on_empty_input(geometry);

    return resolve_dynamic::area<Geometry>::apply(geometry, strategy);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_AREA_HPP
