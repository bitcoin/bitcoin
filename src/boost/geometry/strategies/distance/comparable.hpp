// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DISTANCE_COMPARABLE_HPP
#define BOOST_GEOMETRY_STRATEGIES_DISTANCE_COMPARABLE_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/static_assert.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace distance
{  

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Strategies>
struct comparable
    : public Strategies
{
    comparable() = default;

    explicit comparable(Strategies const& strategies)
        : Strategies(strategies)
    {}

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const& g1, Geometry2 const& g2) const
    {
        return strategy::distance::services::get_comparable
            <
                decltype(Strategies::distance(g1, g2))
            >::apply(Strategies::distance(g1, g2));
    }
};

template <typename Strategies>
inline comparable<Strategies> make_comparable(Strategies const& strategies)
{
    return comparable<Strategies>(strategies);
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace strategies::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DISTANCE_COMPARABLE_HPP
