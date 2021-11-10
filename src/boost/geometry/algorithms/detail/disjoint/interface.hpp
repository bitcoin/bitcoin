// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_INTERFACE_HPP

#include <cstddef>

#include <boost/geometry/algorithms/detail/relate/interface.hpp>
#include <boost/geometry/algorithms/detail/visit.hpp>
#include <boost/geometry/algorithms/dispatch/disjoint.hpp>

#include <boost/geometry/geometries/adapted/boost_variant.hpp> // For backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/relate/services.hpp>


namespace boost { namespace geometry
{

namespace resolve_strategy
{

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct disjoint
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        return dispatch::disjoint
                <
                    Geometry1, Geometry2
                >::apply(geometry1, geometry2, strategy);
    }
};

template <typename Strategy>
struct disjoint<Strategy, false>
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        using strategies::relate::services::strategy_converter;

        return dispatch::disjoint
                <
                    Geometry1, Geometry2
                >::apply(geometry1, geometry2,
                         strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct disjoint<default_strategy, false>
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             default_strategy)
    {
        typedef typename strategies::relate::services::default_strategy
            <
                Geometry1, Geometry2
            >::type strategy_type;

        return dispatch::disjoint
                <
                    Geometry1, Geometry2
                >::apply(geometry1, geometry2, strategy_type());
    }
};

} // namespace resolve_strategy


namespace resolve_dynamic {

template
<
    typename Geometry1, typename Geometry2,
    bool IsDynamic = util::is_dynamic_geometry<Geometry1>::value
                  || util::is_dynamic_geometry<Geometry2>::value,
    bool IsCollection = util::is_geometry_collection<Geometry1>::value
                     || util::is_geometry_collection<Geometry2>::value
>
struct disjoint
{
    template <typename Strategy>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        concepts::check_concepts_and_equal_dimensions
            <
                Geometry1 const,
                Geometry2 const
            >();

        return resolve_strategy::disjoint
            <
                Strategy
            >::apply(geometry1, geometry2, strategy);
    }
};

template <typename Geometry1, typename Geometry2>
struct disjoint<Geometry1, Geometry2, true, false>
{
    template <typename Strategy>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        bool result = true;
        detail::visit([&](auto const& g1, auto const& g2)
        {
            result = disjoint
                <
                    util::remove_cref_t<decltype(g1)>, util::remove_cref_t<decltype(g2)>
                >::apply(g1, g2, strategy);
        }, geometry1, geometry2);
        return result;
    }
};

// TODO: The complexity is quadratic for two GCs
//   Decrease e.g. with spatial index
template <typename Geometry1, typename Geometry2, bool IsDynamic>
struct disjoint<Geometry1, Geometry2, IsDynamic, true>
{
    template <typename Strategy>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        bool result = true;
        detail::visit_breadth_first([&](auto const& g1)
        {
            detail::visit_breadth_first([&](auto const& g2)
            {
                result = disjoint
                    <
                        util::remove_cref_t<decltype(g1)>, util::remove_cref_t<decltype(g2)>
                    >::apply(g1, g2, strategy);
                // If any of the combination intersects then the final result is not disjoint
                return result;
            }, geometry2);
            return result;
        }, geometry1);
        return result;
    }
};

} // namespace resolve_dynamic


/*!
\brief \brief_check2{are disjoint}
\ingroup disjoint
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Strategy \tparam_strategy{Disjoint}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param strategy \param_strategy{disjoint}
\return \return_check2{are disjoint}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/disjoint.qbk]}
*/
template <typename Geometry1, typename Geometry2, typename Strategy>
inline bool disjoint(Geometry1 const& geometry1,
                     Geometry2 const& geometry2,
                     Strategy const& strategy)
{
    return resolve_dynamic::disjoint
            <
                Geometry1, Geometry2
            >::apply(geometry1, geometry2, strategy);
}


/*!
\brief \brief_check2{are disjoint}
\ingroup disjoint
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{are disjoint}

\qbk{[include reference/algorithms/disjoint.qbk]}
\qbk{
[heading Examples]
[disjoint]
[disjoint_output]
}
*/
template <typename Geometry1, typename Geometry2>
inline bool disjoint(Geometry1 const& geometry1,
                     Geometry2 const& geometry2)
{
    return resolve_dynamic::disjoint
            <
                Geometry1, Geometry2
            >::apply(geometry1, geometry2, default_strategy());
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_INTERFACE_HPP
