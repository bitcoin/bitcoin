// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2015 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2014-2015 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_DISTANCE_RESULT_HPP
#define BOOST_GEOMETRY_STRATEGIES_DISTANCE_RESULT_HPP


#include <boost/geometry/algorithms/detail/select_geometry_type.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/distance/services.hpp>
#include <boost/geometry/util/select_most_precise.hpp>
#include <boost/geometry/util/sequence.hpp>
#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{


namespace resolve_strategy
{


// TODO: This utility could be entirely implemented as:
//       decltype(geometry::distance(std::declval<Geometry1>(), std::declval<Geometry2>(), std::declval<Strategy>()))
//       however then the algorithm would have to be compiled.

template
<
    typename Geometry1, typename Geometry2, typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct distance_result_strategy2_type
{
    typedef decltype(std::declval<Strategy>().distance(
        std::declval<Geometry1>(), std::declval<Geometry2>())) type;
};

template <typename Geometry1, typename Geometry2, typename Strategy>
struct distance_result_strategy2_type<Geometry1, Geometry2, Strategy, false>
{
    typedef Strategy type;
};

template
<
    typename Geometry1, typename Geometry2, typename Strategy,
    bool Reverse = reverse_dispatch<Geometry1, Geometry2>::value
>
struct distance_result_strategy_type
    : distance_result_strategy2_type<Geometry1, Geometry2, Strategy>
{};

template <typename Geometry1, typename Geometry2, typename Strategy>
struct distance_result_strategy_type<Geometry1, Geometry2, Strategy, true>
    : distance_result_strategy_type<Geometry2, Geometry1, Strategy, false>
{};


template <typename Geometry1, typename Geometry2, typename Strategy>
struct distance_result
    : strategy::distance::services::return_type
        <
            typename distance_result_strategy_type<Geometry1, Geometry2, Strategy>::type,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >
{};

template <typename Geometry1, typename Geometry2>
struct distance_result<Geometry1, Geometry2, default_strategy>
    : distance_result
        <
            Geometry1,
            Geometry2,
            typename strategies::distance::services::default_strategy
                <
                    Geometry1, Geometry2
                >::type
        >
{};


} // namespace resolve_strategy


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{

template <typename Strategy = geometry::default_strategy>
struct more_precise_distance_result
{
    template <typename Curr, typename Next>
    struct predicate
        : std::is_same
            <
                typename resolve_strategy::distance_result
                    <
                        typename util::sequence_element<0, Curr>::type,
                        typename util::sequence_element<1, Curr>::type,
                        Strategy
                    >::type,
                typename geometry::select_most_precise
                    <
                        typename resolve_strategy::distance_result
                            <
                                typename util::sequence_element<0, Curr>::type,
                                typename util::sequence_element<1, Curr>::type,
                                Strategy
                            >::type,
                        typename resolve_strategy::distance_result
                            <
                                typename util::sequence_element<0, Next>::type,
                                typename util::sequence_element<1, Next>::type,
                                Strategy
                            >::type
                    >::type
            >
    {};
};

}} // namespace detail::distance
#endif //DOXYGEN_NO_DETAIL


namespace resolve_dynamic
{

template
<
    typename Geometry1, typename Geometry2, typename Strategy,
    bool IsDynamicOrCollection = util::is_dynamic_geometry<Geometry1>::value
                              || util::is_dynamic_geometry<Geometry2>::value
                              || util::is_geometry_collection<Geometry1>::value
                              || util::is_geometry_collection<Geometry2>::value
>
struct distance_result
    : resolve_strategy::distance_result
        <
            Geometry1,
            Geometry2,
            Strategy
        >
{};


template <typename Geometry1, typename Geometry2, typename Strategy>
struct distance_result<Geometry1, Geometry2, Strategy, true>
{
    // Select the most precise distance strategy result type
    //   for all variant type combinations.
    // TODO: We should ignore the combinations that are not valid
    //   but is_implemented is not ready for prime time.
    using selected_types = typename detail::select_geometry_types
        <
            Geometry1, Geometry2,
            detail::distance::more_precise_distance_result<Strategy>::template predicate
        >::type;

    using type = typename resolve_strategy::distance_result
        <
            typename util::sequence_element<0, selected_types>::type,
            typename util::sequence_element<1, selected_types>::type,
            Strategy
        >::type;
};


} // namespace resolve_dynamic


/*!
\brief Meta-function defining return type of distance function
\ingroup distance
\note The strategy defines the return-type (so this situation is different
    from length, where distance is sqr/sqrt, but length always squared)
 */
template
<
    typename Geometry1,
    typename Geometry2 = Geometry1,
    typename Strategy = void
>
struct distance_result
    : resolve_dynamic::distance_result<Geometry1, Geometry2, Strategy>
{};


template <typename Geometry1, typename Geometry2>
struct distance_result<Geometry1, Geometry2, void>
    : distance_result<Geometry1, Geometry2, default_strategy>
{};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_DISTANCE_RESULT_HPP
