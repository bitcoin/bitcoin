// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_BUFFER_DISTANCE_ASYMMETRIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_BUFFER_DISTANCE_ASYMMETRIC_HPP

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace buffer
{


/*!
\brief Let the buffer for linestrings be asymmetric
\ingroup strategies
\tparam NumericType \tparam_numeric
\details This strategy can be used as DistanceStrategy for the buffer algorithm.
    It can be applied for (multi)linestrings. It uses a (potentially) different
    distances for left and for right. This means the (multi)linestrings are
    interpreted having a direction.

\qbk{
[heading Example]
[buffer_distance_asymmetric]
[heading Output]
[$img/strategies/buffer_distance_asymmetric.png]
[heading See also]
\* [link geometry.reference.algorithms.buffer.buffer_7_with_strategies buffer (with strategies)]
\* [link geometry.reference.strategies.strategy_buffer_distance_symmetric distance_symmetric]
}
 */
template<typename NumericType>
class distance_asymmetric
{
public :
    //! \brief Constructs the strategy, two distances must be specified
    //! \param left The distance (or radius) of the buffer on the left side
    //! \param right The distance on the right side
    distance_asymmetric(NumericType const& left,
                NumericType const& right)
        : m_left(left)
        , m_right(right)
    {}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    //! Returns the distance-value for the specified side
    template <typename Point>
    inline NumericType apply(Point const& , Point const& ,
                buffer_side_selector side)  const
    {
        NumericType result = side == buffer_side_left ? m_left : m_right;
        return negative() ? math::abs(result) : result;
    }

    //! Used internally, returns -1 for deflate, 1 for inflate
    inline int factor() const
    {
        return negative() ? -1 : 1;
    }

    //! Returns true if both distances are negative
    inline bool negative() const
    {
        return m_left < 0 && m_right < 0;
    }

    //! Returns the max distance distance up to the buffer will reach
    template <typename JoinStrategy, typename EndStrategy>
    inline NumericType max_distance(JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy) const
    {
        boost::ignore_unused(join_strategy, end_strategy);

        NumericType const left = geometry::math::abs(m_left);
        NumericType const right = geometry::math::abs(m_right);
        NumericType const dist = (std::max)(left, right);
        return (std::max)(join_strategy.max_distance(dist),
                          end_strategy.max_distance(dist));
    }

    //! Returns the distance at which the input is simplified before the buffer process
    inline NumericType simplify_distance() const
    {
        NumericType const left = geometry::math::abs(m_left);
        NumericType const right = geometry::math::abs(m_right);
        return (std::min)(left, right) / 1000.0;
    }

#endif // DOXYGEN_SHOULD_SKIP_THIS

private :
    NumericType m_left;
    NumericType m_right;
};


}} // namespace strategy::buffer


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_BUFFER_DISTANCE_ASYMMETRIC_HPP
