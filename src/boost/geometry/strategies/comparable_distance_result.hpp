// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_COMPARABLE_DISTANCE_RESULT_HPP
#define BOOST_GEOMETRY_STRATEGIES_COMPARABLE_DISTANCE_RESULT_HPP


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
//       decltype(geometry::comparable_distance(std::declval<Geometry1>(), std::declval<Geometry2>(), std::declval<Strategy>()))
//       however then the algorithm would have to be compiled.


template
<
    typename Geometry1, typename Geometry2, typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct comparable_distance_strategy2_type
{
    typedef decltype(std::declval<Strategy>().distance(
        std::declval<Geometry1>(), std::declval<Geometry2>())) type;
};

template <typename Geometry1, typename Geometry2, typename Strategy>
struct comparable_distance_strategy2_type<Geometry1, Geometry2, Strategy, false>
{
    typedef Strategy type;
};

template
<
    typename Geometry1, typename Geometry2, typename Strategy,
    bool Reverse = reverse_dispatch<Geometry1, Geometry2>::type::value
>
struct comparable_distance_strategy_type
    : strategy::distance::services::comparable_type
        <
            typename comparable_distance_strategy2_type
                <
                    Geometry1, Geometry2, Strategy
                >::type
        >
{};

template <typename Geometry1, typename Geometry2, typename Strategy>
struct comparable_distance_strategy_type<Geometry1, Geometry2, Strategy, true>
    : comparable_distance_strategy_type<Geometry2, Geometry1, Strategy, false>
{};

    
template <typename Geometry1, typename Geometry2, typename Strategy>
struct comparable_distance_result
    : strategy::distance::services::return_type
        <
            typename comparable_distance_strategy_type<Geometry1, Geometry2, Strategy>::type,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >
{};

template <typename Geometry1, typename Geometry2>
struct comparable_distance_result<Geometry1, Geometry2, default_strategy>
    : comparable_distance_result
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
struct more_precise_comparable_distance_result
{
    template <typename Curr, typename Next>
    struct predicate
        : std::is_same
            <
                typename resolve_strategy::comparable_distance_result
                    <
                        typename util::sequence_element<0, Curr>::type,
                        typename util::sequence_element<1, Curr>::type,
                        Strategy
                    >::type,
                typename geometry::select_most_precise
                    <
                        typename resolve_strategy::comparable_distance_result
                            <
                                typename util::sequence_element<0, Curr>::type,
                                typename util::sequence_element<1, Curr>::type,
                                Strategy
                            >::type,
                        typename resolve_strategy::comparable_distance_result
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
struct comparable_distance_result
    : resolve_strategy::comparable_distance_result
        <
            Geometry1,
            Geometry2,
            Strategy
        >
{};

template <typename Geometry1, typename Geometry2, typename Strategy>
struct comparable_distance_result<Geometry1, Geometry2, Strategy, true>
{
    // Select the most precise distance strategy result type
    //   for all variant type combinations.
    // TODO: We should ignore the combinations that are not valid
    //   but is_implemented is not ready for prime time.
    using selected_types = typename detail::select_geometry_types
        <
            Geometry1, Geometry2,
            detail::distance::more_precise_comparable_distance_result<Strategy>::template predicate
        >::type;

    using type = typename resolve_strategy::comparable_distance_result
        <
            typename util::sequence_element<0, selected_types>::type,
            typename util::sequence_element<1, selected_types>::type,
            Strategy
        >::type;
};


} // namespace resolve_dynamic


/*!
\brief Meta-function defining return type of comparable_distance function
\ingroup distance
*/
template
<
    typename Geometry1,
    typename Geometry2 = Geometry1,
    typename Strategy = void
>
struct comparable_distance_result
    : resolve_dynamic::comparable_distance_result
        <
            Geometry1, Geometry2, Strategy
        >
{};

template <typename Geometry1, typename Geometry2>
struct comparable_distance_result<Geometry1, Geometry2, void>
    : comparable_distance_result<Geometry1, Geometry2, default_strategy>
{};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_COMPARABLE_DISTANCE_RESULT_HPP
