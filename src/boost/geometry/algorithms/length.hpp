// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_LENGTH_HPP
#define BOOST_GEOMETRY_ALGORITHMS_LENGTH_HPP

#include <iterator>

#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/detail/calculate_null.hpp>
#include <boost/geometry/algorithms/detail/dummy_geometries.hpp>
#include <boost/geometry/algorithms/detail/multi_sum.hpp>
// #include <boost/geometry/algorithms/detail/throw_on_empty_input.hpp>
#include <boost/geometry/algorithms/detail/visit.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>

#include <boost/geometry/geometries/adapted/boost_variant.hpp> // For backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/default_length_result.hpp> // TODO: Move to algorithms
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/length/cartesian.hpp>
#include <boost/geometry/strategies/length/geographic.hpp>
#include <boost/geometry/strategies/length/spherical.hpp>

#include <boost/geometry/views/closeable_view.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace length
{


template<typename Segment>
struct segment_length
{
    template <typename Strategies>
    static inline typename default_length_result<Segment>::type
    apply(Segment const& segment, Strategies const& strategies)
    {
        typedef typename point_type<Segment>::type point_type;
        point_type p1, p2;
        geometry::detail::assign_point_from_index<0>(segment, p1);
        geometry::detail::assign_point_from_index<1>(segment, p2);
        return strategies.distance(p1, p2).apply(p1, p2);
    }
};

/*!
\brief Internal, calculates length of a linestring using iterator pairs and
    specified strategy
\note for_each could be used here, now that point_type is changed by boost
    range iterator
*/
template<typename Range, closure_selector Closure>
struct range_length
{
    typedef typename default_length_result<Range>::type return_type;

    template <typename Strategies>
    static inline return_type
    apply(Range const& range, Strategies const& strategies)
    {
        return_type sum = return_type();
        detail::closed_view<Range const> const view(range);
        auto it = boost::begin(view);
        auto const end = boost::end(view);
        if (it != end)
        {
            auto const strategy = strategies.distance(dummy_point(), dummy_point());

            for(auto previous = it++; it != end; ++previous, ++it)
            {
                // Add point-point distance using the return type belonging
                // to strategy
                sum += strategy.apply(*previous, *it);
            }
        }

        return sum;
    }
};


}} // namespace detail::length
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct length : detail::calculate_null
{
    typedef typename default_length_result<Geometry>::type return_type;

    template <typename Strategy>
    static inline return_type apply(Geometry const& geometry, Strategy const& strategy)
    {
        return calculate_null::apply<return_type>(geometry, strategy);
    }
};


template <typename Geometry>
struct length<Geometry, linestring_tag>
    : detail::length::range_length<Geometry, closed>
{};


// RING: length is currently 0; it might be argued that it is the "perimeter"


template <typename Geometry>
struct length<Geometry, segment_tag>
    : detail::length::segment_length<Geometry>
{};


template <typename MultiLinestring>
struct length<MultiLinestring, multi_linestring_tag> : detail::multi_sum
{
    template <typename Strategy>
    static inline typename default_length_result<MultiLinestring>::type
    apply(MultiLinestring const& multi, Strategy const& strategy)
    {
        return multi_sum::apply
               <
                   typename default_length_result<MultiLinestring>::type,
                   detail::length::range_length
                   <
                       typename boost::range_value<MultiLinestring>::type,
                       closed // no need to close it explicitly
                   >
               >(multi, strategy);

    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy {

template
<
    typename Strategies,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategies>::value
>
struct length
{
    template <typename Geometry>
    static inline typename default_length_result<Geometry>::type
    apply(Geometry const& geometry, Strategies const& strategies)
    {
        return dispatch::length<Geometry>::apply(geometry, strategies);
    }
};

template <typename Strategy>
struct length<Strategy, false>
{
    template <typename Geometry>
    static inline typename default_length_result<Geometry>::type
    apply(Geometry const& geometry, Strategy const& strategy)
    {
        using strategies::length::services::strategy_converter;
        return dispatch::length<Geometry>::apply(
                geometry, strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct length<default_strategy, false>
{
    template <typename Geometry>
    static inline typename default_length_result<Geometry>::type
    apply(Geometry const& geometry, default_strategy const&)
    {
        typedef typename strategies::length::services::default_strategy
            <
                Geometry
            >::type strategies_type;

        return dispatch::length<Geometry>::apply(geometry, strategies_type());
    }
};

} // namespace resolve_strategy


namespace resolve_dynamic {

template <typename Geometry, typename Tag = typename geometry::tag<Geometry>::type>
struct length
{
    template <typename Strategy>
    static inline typename default_length_result<Geometry>::type
    apply(Geometry const& geometry, Strategy const& strategy)
    {
        return resolve_strategy::length<Strategy>::apply(geometry, strategy);
    }
};

template <typename Geometry>
struct length<Geometry, dynamic_geometry_tag>
{
    template <typename Strategy>
    static inline typename default_length_result<Geometry>::type
        apply(Geometry const& geometry, Strategy const& strategy)
    {
        typename default_length_result<Geometry>::type result = 0;
        traits::visit<Geometry>::apply([&](auto const& g)
        {
            result = length<util::remove_cref_t<decltype(g)>>::apply(g, strategy);
        }, geometry);
        return result;
    }
};

template <typename Geometry>
struct length<Geometry, geometry_collection_tag>
{
    template <typename Strategy>
    static inline typename default_length_result<Geometry>::type
        apply(Geometry const& geometry, Strategy const& strategy)
    {
        typename default_length_result<Geometry>::type result = 0;
        detail::visit_breadth_first([&](auto const& g)
        {
            result += length<util::remove_cref_t<decltype(g)>>::apply(g, strategy);
            return true;
        }, geometry);
        return result;
    }
};

} // namespace resolve_dynamic


/*!
\brief \brief_calc{length}
\ingroup length
\details \details_calc{length, length (the sum of distances between consecutive points)}. \details_default_strategy
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{length}

\qbk{[include reference/algorithms/length.qbk]}
\qbk{[length] [length_output]}
 */
template<typename Geometry>
inline typename default_length_result<Geometry>::type
length(Geometry const& geometry)
{
    concepts::check<Geometry const>();

    // detail::throw_on_empty_input(geometry);

    return resolve_dynamic::length<Geometry>::apply(geometry, default_strategy());
}


/*!
\brief \brief_calc{length} \brief_strategy
\ingroup length
\details \details_calc{length, length (the sum of distances between consecutive points)} \brief_strategy. \details_strategy_reasons
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{distance}
\param geometry \param_geometry
\param strategy \param_strategy{distance}
\return \return_calc{length}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/length.qbk]}
\qbk{[length_with_strategy] [length_with_strategy_output]}
 */
template<typename Geometry, typename Strategy>
inline typename default_length_result<Geometry>::type
length(Geometry const& geometry, Strategy const& strategy)
{
    concepts::check<Geometry const>();

    // detail::throw_on_empty_input(geometry);

    return resolve_dynamic::length<Geometry>::apply(geometry, strategy);
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_LENGTH_HPP
