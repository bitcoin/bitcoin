// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CENTROID_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CENTROID_HPP


#include <cstddef>

#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/throw_exception.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/exception.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/centroid/translating_transformer.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/is_empty.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/centroid/cartesian.hpp>
#include <boost/geometry/strategies/centroid/geographic.hpp>
#include <boost/geometry/strategies/centroid/spherical.hpp>
#include <boost/geometry/strategies/concepts/centroid_concept.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>

#include <boost/geometry/util/algorithm.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>

#include <boost/geometry/views/closeable_view.hpp>


namespace boost { namespace geometry
{


#if ! defined(BOOST_GEOMETRY_CENTROID_NO_THROW)

/*!
\brief Centroid Exception
\ingroup centroid
\details The centroid_exception is thrown if the free centroid function is called with
    geometries for which the centroid cannot be calculated. For example: a linestring
    without points, a polygon without points, an empty multi-geometry.
\qbk{
[heading See also]
\* [link geometry.reference.algorithms.centroid the centroid function]
}

 */
class centroid_exception : public geometry::exception
{
public:

    /*!
    \brief The default constructor
    */
    inline centroid_exception() {}

    /*!
    \brief Returns the explanatory string.
    \return Pointer to a null-terminated string with explanatory information.
    */
    virtual char const* what() const throw()
    {
        return "Boost.Geometry Centroid calculation exception";
    }
};

#endif


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace centroid
{

struct centroid_point
{
    template<typename Point, typename PointCentroid, typename Strategy>
    static inline void apply(Point const& point, PointCentroid& centroid,
            Strategy const&)
    {
        geometry::convert(point, centroid);
    }
};

struct centroid_indexed
{
    template<typename Indexed, typename Point, typename Strategy>
    static inline void apply(Indexed const& indexed, Point& centroid,
            Strategy const&)
    {
        typedef typename select_coordinate_type
            <
                Indexed, Point
            >::type coordinate_type;

        detail::for_each_dimension<Indexed>([&](auto dimension)
        {
            coordinate_type const c1 = get<min_corner, dimension>(indexed);
            coordinate_type const c2 = get<max_corner, dimension>(indexed);
            coordinate_type const two = 2;
            set<dimension>(centroid, (c1 + c2) / two);
        });
    }
};


// There is one thing where centroid is different from e.g. within.
// If the ring has only one point, it might make sense that
// that point is the centroid.
template<typename Point, typename Range>
inline bool range_ok(Range const& range, Point& centroid)
{
    std::size_t const n = boost::size(range);
    if (n > 1)
    {
        return true;
    }
    else if (n <= 0)
    {
#if ! defined(BOOST_GEOMETRY_CENTROID_NO_THROW)
        BOOST_THROW_EXCEPTION(centroid_exception());
#else
        return false;
#endif
    }
    else // if (n == 1)
    {
        // Take over the first point in a "coordinate neutral way"
        geometry::convert(*boost::begin(range), centroid);
        return false;
    }
    //return true; // unreachable
}

/*!
    \brief Calculate the centroid of a Ring or a Linestring.
*/
struct centroid_range_state
{
    template<typename Ring, typename PointTransformer, typename Strategy, typename State>
    static inline void apply(Ring const& ring,
                             PointTransformer const& transformer,
                             Strategy const& strategy,
                             State& state)
    {
        boost::ignore_unused(strategy);

        detail::closed_view<Ring const> const view(ring);
        auto it = boost::begin(view);
        auto const end = boost::end(view);

        if (it != end)
        {
            typename PointTransformer::result_type
                previous_pt = transformer.apply(*it);

            for ( ++it ; it != end ; ++it)
            {
                typename PointTransformer::result_type
                    pt = transformer.apply(*it);

                using point_type = typename geometry::point_type<Ring const>::type;
                strategy.apply(static_cast<point_type const&>(previous_pt),
                               static_cast<point_type const&>(pt),
                               state);

                previous_pt = pt;
            }
        }
    }
};

struct centroid_range
{
    template<typename Range, typename Point, typename Strategy>
    static inline bool apply(Range const& range, Point& centroid,
                             Strategy const& strategy)
    {
        if (range_ok(range, centroid))
        {
            // prepare translation transformer
            translating_transformer<Range> transformer(*boost::begin(range));
            
            typename Strategy::template state_type
                <
                    typename geometry::point_type<Range>::type,
                    Point
                >::type state;

            centroid_range_state::apply(range, transformer, strategy, state);
            
            if ( strategy.result(state, centroid) )
            {
                // translate the result back
                transformer.apply_reverse(centroid);
                return true;
            }
        }

        return false;
    }
};


/*!
    \brief Centroid of a polygon.
    \note Because outer ring is clockwise, inners are counter clockwise,
    triangle approach is OK and works for polygons with rings.
*/
struct centroid_polygon_state
{
    template<typename Polygon, typename PointTransformer, typename Strategy, typename State>
    static inline void apply(Polygon const& poly,
                             PointTransformer const& transformer,
                             Strategy const& strategy,
                             State& state)
    {
        centroid_range_state::apply(exterior_ring(poly), transformer, strategy, state);

        typename interior_return_type<Polygon const>::type
            rings = interior_rings(poly);

        for (typename detail::interior_iterator<Polygon const>::type
                it = boost::begin(rings); it != boost::end(rings); ++it)
        {
            centroid_range_state::apply(*it, transformer, strategy, state);
        }
    }
};

struct centroid_polygon
{
    template<typename Polygon, typename Point, typename Strategy>
    static inline bool apply(Polygon const& poly, Point& centroid,
                             Strategy const& strategy)
    {
        if (range_ok(exterior_ring(poly), centroid))
        {
            // prepare translation transformer
            translating_transformer<Polygon>
                transformer(*boost::begin(exterior_ring(poly)));
            
            typename Strategy::template state_type
                <
                    typename geometry::point_type<Polygon>::type,
                    Point
                >::type state;

            centroid_polygon_state::apply(poly, transformer, strategy, state);
            
            if ( strategy.result(state, centroid) )
            {
                // translate the result back
                transformer.apply_reverse(centroid);
                return true;
            }
        }

        return false;
    }
};


/*!
    \brief Building block of a multi-point, to be used as Policy in the
        more generec centroid_multi
*/
struct centroid_multi_point_state
{
    template <typename Point, typename PointTransformer, typename Strategy, typename State>
    static inline void apply(Point const& point,
                             PointTransformer const& transformer,
                             Strategy const& strategy,
                             State& state)
    {
        boost::ignore_unused(strategy);
        strategy.apply(static_cast<Point const&>(transformer.apply(point)),
                       state);
    }
};


/*!
    \brief Generic implementation which calls a policy to calculate the
        centroid of the total of its single-geometries
    \details The Policy is, in general, the single-version, with state. So
        detail::centroid::centroid_polygon_state is used as a policy for this
        detail::centroid::centroid_multi

*/
template <typename Policy>
struct centroid_multi
{
    template <typename Multi, typename Point, typename Strategy>
    static inline bool apply(Multi const& multi,
                             Point& centroid,
                             Strategy const& strategy)
    {
#if ! defined(BOOST_GEOMETRY_CENTROID_NO_THROW)
        // If there is nothing in any of the ranges, it is not possible
        // to calculate the centroid
        if (geometry::is_empty(multi))
        {
            BOOST_THROW_EXCEPTION(centroid_exception());
        }
#endif

        // prepare translation transformer
        translating_transformer<Multi> transformer(multi);

        typename Strategy::template state_type
            <
                typename geometry::point_type<Multi>::type,
                Point
            >::type state;

        for (typename boost::range_iterator<Multi const>::type
                it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            Policy::apply(*it, transformer, strategy, state);
        }

        if ( strategy.result(state, centroid) )
        {
            // translate the result back
            transformer.apply_reverse(centroid);
            return true;
        }
        
        return false;
    }
};


template <typename Algorithm>
struct centroid_linear_areal
{
    template <typename Geometry, typename Point, typename Strategies>
    static inline void apply(Geometry const& geom,
                             Point& centroid,
                             Strategies const& strategies)
    {
        if ( ! Algorithm::apply(geom, centroid, strategies.centroid(geom)) )
        {
            geometry::point_on_border(centroid, geom);
        }
    }
};

template <typename Algorithm>
struct centroid_pointlike
{
    template <typename Geometry, typename Point, typename Strategies>
    static inline void apply(Geometry const& geom,
                             Point& centroid,
                             Strategies const& strategies)
    {
        Algorithm::apply(geom, centroid, strategies.centroid(geom));
    }
};


}} // namespace detail::centroid
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct centroid: not_implemented<Tag>
{};

template <typename Geometry>
struct centroid<Geometry, point_tag>
    : detail::centroid::centroid_point
{};

template <typename Box>
struct centroid<Box, box_tag>
    : detail::centroid::centroid_indexed
{};

template <typename Segment>
struct centroid<Segment, segment_tag>
    : detail::centroid::centroid_indexed
{};

template <typename Ring>
struct centroid<Ring, ring_tag>
    : detail::centroid::centroid_linear_areal
        <
            detail::centroid::centroid_range
        >
{};

template <typename Linestring>
struct centroid<Linestring, linestring_tag>
    : detail::centroid::centroid_linear_areal
        <
            detail::centroid::centroid_range
        >
{};

template <typename Polygon>
struct centroid<Polygon, polygon_tag>
    : detail::centroid::centroid_linear_areal
        <
            detail::centroid::centroid_polygon
        >
{};

template <typename MultiLinestring>
struct centroid<MultiLinestring, multi_linestring_tag>
    : detail::centroid::centroid_linear_areal
        <
            detail::centroid::centroid_multi
            <
                detail::centroid::centroid_range_state
            >
        >
{};

template <typename MultiPolygon>
struct centroid<MultiPolygon, multi_polygon_tag>
    : detail::centroid::centroid_linear_areal
        <
            detail::centroid::centroid_multi
            <
                detail::centroid::centroid_polygon_state
            >
        >
{};

template <typename MultiPoint>
struct centroid<MultiPoint, multi_point_tag>
    : detail::centroid::centroid_pointlike
        <
            detail::centroid::centroid_multi
            <
                detail::centroid::centroid_multi_point_state
            >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy {

template
<
    typename Strategies,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategies>::value
>
struct centroid
{
    template <typename Geometry, typename Point>
    static inline void apply(Geometry const& geometry, Point& out, Strategies const& strategies)
    {
        dispatch::centroid<Geometry>::apply(geometry, out, strategies);
    }
};

template <typename Strategy>
struct centroid<Strategy, false>
{
    template <typename Geometry, typename Point>
    static inline void apply(Geometry const& geometry, Point& out, Strategy const& strategy)
    {
        using strategies::centroid::services::strategy_converter;
        dispatch::centroid
            <
                Geometry
            >::apply(geometry, out, strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct centroid<default_strategy, false>
{
    template <typename Geometry, typename Point>
    static inline void apply(Geometry const& geometry, Point& out, default_strategy)
    {
        typedef typename strategies::centroid::services::default_strategy
            <
                Geometry
            >::type strategies_type;

        dispatch::centroid<Geometry>::apply(geometry, out, strategies_type());
    }
};

} // namespace resolve_strategy


namespace resolve_variant {

template <typename Geometry>
struct centroid
{
    template <typename Point, typename Strategy>
    static inline void apply(Geometry const& geometry, Point& out, Strategy const& strategy)
    {
        concepts::check_concepts_and_equal_dimensions<Point, Geometry const>();
        resolve_strategy::centroid<Strategy>::apply(geometry, out, strategy);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct centroid<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename Point, typename Strategy>
    struct visitor: boost::static_visitor<void>
    {
        Point& m_out;
        Strategy const& m_strategy;

        visitor(Point& out, Strategy const& strategy)
        : m_out(out), m_strategy(strategy)
        {}

        template <typename Geometry>
        void operator()(Geometry const& geometry) const
        {
            centroid<Geometry>::apply(geometry, m_out, m_strategy);
        }
    };

    template <typename Point, typename Strategy>
    static inline void
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          Point& out,
          Strategy const& strategy)
    {
        boost::apply_visitor(visitor<Point, Strategy>(out, strategy), geometry);
    }
};

} // namespace resolve_variant


/*!
\brief \brief_calc{centroid} \brief_strategy
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_strategy_reasons
\tparam Geometry \tparam_geometry
\tparam Point \tparam_point
\tparam Strategy \tparam_strategy{Centroid}
\param geometry \param_geometry
\param c \param_point \param_set{centroid}
\param strategy \param_strategy{centroid}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/centroid.qbk]}
\qbk{[include reference/algorithms/centroid_strategies.qbk]}
}

*/
template<typename Geometry, typename Point, typename Strategy>
inline void centroid(Geometry const& geometry, Point& c,
        Strategy const& strategy)
{
    resolve_variant::centroid<Geometry>::apply(geometry, c, strategy);
}


/*!
\brief \brief_calc{centroid}
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_default_strategy
\tparam Geometry \tparam_geometry
\tparam Point \tparam_point
\param geometry \param_geometry
\param c The calculated centroid will be assigned to this point reference

\qbk{[include reference/algorithms/centroid.qbk]}
\qbk{
[heading Example]
[centroid]
[centroid_output]
}
 */
template<typename Geometry, typename Point>
inline void centroid(Geometry const& geometry, Point& c)
{
    geometry::centroid(geometry, c, default_strategy());
}


/*!
\brief \brief_calc{centroid}
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_return{centroid}.
\tparam Point \tparam_point
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{centroid}

\qbk{[include reference/algorithms/centroid.qbk]}
 */
template<typename Point, typename Geometry>
inline Point return_centroid(Geometry const& geometry)
{
    Point c;
    geometry::centroid(geometry, c);
    return c;
}

/*!
\brief \brief_calc{centroid} \brief_strategy
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_return{centroid}. \details_strategy_reasons
\tparam Point \tparam_point
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{centroid}
\param geometry \param_geometry
\param strategy \param_strategy{centroid}
\return \return_calc{centroid}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/centroid.qbk]}
\qbk{[include reference/algorithms/centroid_strategies.qbk]}
 */
template<typename Point, typename Geometry, typename Strategy>
inline Point return_centroid(Geometry const& geometry, Strategy const& strategy)
{
    Point c;
    geometry::centroid(geometry, c, strategy);
    return c;
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_CENTROID_HPP
