// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017, 2019.
// Modifications copyright (c) 2017, 2019, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_POLICIES_COMPARE_HPP
#define BOOST_GEOMETRY_POLICIES_COMPARE_HPP


#include <cstddef>

#include <boost/geometry/strategies/compare.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{


/*!
\brief Less functor, to sort points in ascending order.
\ingroup compare
\details This functor compares points and orders them on x,
    then on y, then on z coordinate.
\tparam Point the geometry
\tparam Dimension the dimension to sort on, defaults to -1,
    indicating ALL dimensions. That's to say, first on x,
    on equal x-es then on y, etc.
    If a dimension is specified, only that dimension is considered
*/
template
<
    typename Point = void,
    int Dimension = -1,
    typename CSTag = void
>
struct less
{
    typedef Point first_argument_type;
    typedef Point second_argument_type;
    typedef bool result_type;

    inline bool operator()(Point const& left, Point const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::less,
                Point, Point,
                Dimension,
                CSTag, CSTag
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <int Dimension, typename CSTag>
struct less<void, Dimension, CSTag>
{
    typedef bool result_type;

    template <typename Point1, typename Point2>
    inline bool operator()(Point1 const& left, Point2 const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::less,
                Point1, Point2,
                Dimension,
                CSTag, CSTag
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <typename Point, int Dimension>
struct less<Point, Dimension, void>
{
    typedef Point first_argument_type;
    typedef Point second_argument_type;
    typedef bool result_type;

    inline bool operator()(Point const& left, Point const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::less,
                Point, Point,
                Dimension
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <int Dimension>
struct less<void, Dimension, void>
{
    typedef bool result_type;

    template <typename Point1, typename Point2>
    inline bool operator()(Point1 const& left, Point2 const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::less,
                Point1, Point2,
                Dimension
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};


/*!
\brief Greater functor
\ingroup compare
\details Can be used to sort points in reverse order
\see Less functor
*/
template
<
    typename Point = void,
    int Dimension = -1,
    typename CSTag = void
>
struct greater
{
    typedef Point first_argument_type;
    typedef Point second_argument_type;
    typedef bool result_type;

    bool operator()(Point const& left, Point const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::greater,
                Point, Point,
                Dimension,
                CSTag, CSTag
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <int Dimension, typename CSTag>
struct greater<void, Dimension, CSTag>
{
    typedef bool result_type;

    template <typename Point1, typename Point2>
    bool operator()(Point1 const& left, Point2 const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::greater,
                Point1, Point2,
                Dimension,
                CSTag, CSTag
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <typename Point, int Dimension>
struct greater<Point, Dimension, void>
{
    typedef Point first_argument_type;
    typedef Point second_argument_type;
    typedef bool result_type;

    bool operator()(Point const& left, Point const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::greater,
                Point, Point,
                Dimension
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <int Dimension>
struct greater<void, Dimension, void>
{
    typedef bool result_type;

    template <typename Point1, typename Point2>
    bool operator()(Point1 const& left, Point2 const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::greater,
                Point1, Point2,
                Dimension
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};


/*!
\brief Equal To functor, to compare if points are equal
\ingroup compare
\tparam Geometry the geometry
\tparam Dimension the dimension to compare on, defaults to -1,
    indicating ALL dimensions.
    If a dimension is specified, only that dimension is considered
*/
template
<
    typename Point,
    int Dimension = -1,
    typename CSTag = void
>
struct equal_to
{
    typedef Point first_argument_type;
    typedef Point second_argument_type;
    typedef bool result_type;

    bool operator()(Point const& left, Point const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::equal_to,
                Point, Point,
                Dimension,
                CSTag, CSTag
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <int Dimension, typename CSTag>
struct equal_to<void, Dimension, CSTag>
{
    typedef bool result_type;

    template <typename Point1, typename Point2>
    bool operator()(Point1 const& left, Point2 const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::equal_to,
                Point1, Point2,
                Dimension,
                CSTag, CSTag
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <typename Point, int Dimension>
struct equal_to<Point, Dimension, void>
{
    typedef Point first_argument_type;
    typedef Point second_argument_type;
    typedef bool result_type;

    bool operator()(Point const& left, Point const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::equal_to,
                Point, Point,
                Dimension
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};

template <int Dimension>
struct equal_to<void, Dimension, void>
{
    typedef bool result_type;

    template <typename Point1, typename Point2>
    bool operator()(Point1 const& left, Point2 const& right) const
    {
        typedef typename strategy::compare::services::default_strategy
            <
                strategy::compare::equal_to,
                Point1, Point2,
                Dimension
            >::type strategy_type;

        return strategy_type::apply(left, right);
    }
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_POLICIES_COMPARE_HPP
