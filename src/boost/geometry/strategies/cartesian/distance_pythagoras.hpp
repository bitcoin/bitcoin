// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2018.
// Modifications copyright (c) 2018, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PYTHAGORAS_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PYTHAGORAS_HPP


#include <boost/geometry/core/access.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>

#include <boost/geometry/strategies/distance.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/calculation_type.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace distance
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <size_t I, typename T>
struct compute_pythagoras
{
    template <typename Point1, typename Point2>
    static inline T apply(Point1 const& p1, Point2 const& p2)
    {
        T const c1 = boost::numeric_cast<T>(get<I-1>(p1));
        T const c2 = boost::numeric_cast<T>(get<I-1>(p2));
        T const d = c1 - c2;
        return d * d + compute_pythagoras<I-1, T>::apply(p1, p2);
    }
};

template <typename T>
struct compute_pythagoras<0, T>
{
    template <typename Point1, typename Point2>
    static inline T apply(Point1 const&, Point2 const&)
    {
        return boost::numeric_cast<T>(0);
    }
};

}
#endif // DOXYGEN_NO_DETAIL


namespace comparable
{

/*!
\brief Strategy to calculate comparable distance between two points
\ingroup strategies
\tparam Point1 \tparam_first_point
\tparam Point2 \tparam_second_point
\tparam CalculationType \tparam_calculation
*/
template <typename CalculationType = void>
class pythagoras
{
public :

    template <typename Point1, typename Point2>
    struct calculation_type
        : util::calculation_type::geometric::binary
          <
              Point1,
              Point2,
              CalculationType,
              double,
              double
          >
    {};

    template <typename Point1, typename Point2>
    static inline typename calculation_type<Point1, Point2>::type
    apply(Point1 const& p1, Point2 const& p2)
    {
        BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point1>) );
        BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point2>) );

        // Calculate distance using Pythagoras
        // (Leave comment above for Doxygen)

        assert_dimension_equal<Point1, Point2>();

        return detail::compute_pythagoras
            <
                dimension<Point1>::value,
                typename calculation_type<Point1, Point2>::type
            >::apply(p1, p2);
    }
};

} // namespace comparable


/*!
\brief Strategy to calculate the distance between two points
\ingroup strategies
\tparam CalculationType \tparam_calculation

\qbk{
[heading Notes]
[note Can be used for points with two\, three or more dimensions]
[heading See also]
[link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
}

*/
template
<
    typename CalculationType = void
>
class pythagoras
{
public :

    template <typename P1, typename P2>
    struct calculation_type
        : util::calculation_type::geometric::binary
          <
              P1,
              P2,
              CalculationType,
              double,
              double // promote integer to double
          >
    {};

    /*!
    \brief applies the distance calculation using pythagoras
    \return the calculated distance (including taking the square root)
    \param p1 first point
    \param p2 second point
    */
    template <typename P1, typename P2>
    static inline typename calculation_type<P1, P2>::type
    apply(P1 const& p1, P2 const& p2)
    {
        // The cast is necessary for MSVC which considers sqrt __int64 as an ambiguous call
        return math::sqrt
            (
                 boost::numeric_cast<typename calculation_type<P1, P2>::type>
                    (
                        comparable::pythagoras<CalculationType>::apply(p1, p2)
                    )
            );
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType>
struct tag<pythagoras<CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename CalculationType, typename P1, typename P2>
struct return_type<distance::pythagoras<CalculationType>, P1, P2>
    : pythagoras<CalculationType>::template calculation_type<P1, P2>
{};


template <typename CalculationType>
struct comparable_type<pythagoras<CalculationType> >
{
    typedef comparable::pythagoras<CalculationType> type;
};


template <typename CalculationType>
struct get_comparable<pythagoras<CalculationType> >
{
    typedef comparable::pythagoras<CalculationType> comparable_type;
public :
    static inline comparable_type apply(pythagoras<CalculationType> const& )
    {
        return comparable_type();
    }
};


template <typename CalculationType, typename Point1, typename Point2>
struct result_from_distance<pythagoras<CalculationType>, Point1, Point2>
{
private :
    typedef typename return_type<pythagoras<CalculationType>, Point1, Point2>::type return_type;
public :
    template <typename T>
    static inline return_type apply(pythagoras<CalculationType> const& , T const& value)
    {
        return return_type(value);
    }
};


// Specializations for comparable::pythagoras
template <typename CalculationType>
struct tag<comparable::pythagoras<CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename CalculationType, typename P1, typename P2>
struct return_type<comparable::pythagoras<CalculationType>, P1, P2>
    : comparable::pythagoras<CalculationType>::template calculation_type<P1, P2>
{};




template <typename CalculationType>
struct comparable_type<comparable::pythagoras<CalculationType> >
{
    typedef comparable::pythagoras<CalculationType> type;
};


template <typename CalculationType>
struct get_comparable<comparable::pythagoras<CalculationType> >
{
    typedef comparable::pythagoras<CalculationType> comparable_type;
public :
    static inline comparable_type apply(comparable::pythagoras<CalculationType> const& )
    {
        return comparable_type();
    }
};


template <typename CalculationType, typename Point1, typename Point2>
struct result_from_distance<comparable::pythagoras<CalculationType>, Point1, Point2>
{
private :
    typedef typename return_type<comparable::pythagoras<CalculationType>, Point1, Point2>::type return_type;
public :
    template <typename T>
    static inline return_type apply(comparable::pythagoras<CalculationType> const& , T const& value)
    {
        return_type const v = value;
        return v * v;
    }
};


template <typename Point1, typename Point2>
struct default_strategy
    <
        point_tag, point_tag, Point1, Point2, cartesian_tag, cartesian_tag
    >
{
    typedef pythagoras<> type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PYTHAGORAS_HPP
