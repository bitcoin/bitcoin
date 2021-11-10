// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_HPP

#include <algorithm>
#include <type_traits>

#include <boost/config.hpp>
#include <boost/concept_check.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/formulas/spherical.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/concepts/distance_concept.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>
#include <boost/geometry/strategies/spherical/intersection.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
#  include <boost/geometry/io/dsv/write.hpp>
#endif


namespace boost { namespace geometry
{

namespace strategy { namespace distance
{


namespace comparable
{

/*
  Given a spherical segment AB and a point D, we are interested in
  computing the distance of D from AB. This is usually known as the
  cross track distance.

  If the projection (along great circles) of the point D lies inside
  the segment AB, then the distance (cross track error) XTD is given
  by the formula (see http://williams.best.vwh.net/avform.htm#XTE):

  XTD = asin( sin(dist_AD) * sin(crs_AD-crs_AB) )

  where dist_AD is the great circle distance between the points A and
  B, and crs_AD, crs_AB is the course (bearing) between the points A,
  D and A, B, respectively.

  If the point D does not project inside the arc AB, then the distance
  of D from AB is the minimum of the two distances dist_AD and dist_BD.

  Our reference implementation for this procedure is listed below
  (this was the old Boost.Geometry implementation of the cross track distance),
  where:
  * The member variable m_strategy is the underlying haversine strategy.
  * p stands for the point D.
  * sp1 stands for the segment endpoint A.
  * sp2 stands for the segment endpoint B.

  ================= reference implementation -- start =================

  return_type d1 = m_strategy.apply(sp1, p);
  return_type d3 = m_strategy.apply(sp1, sp2);

  if (geometry::math::equals(d3, 0.0))
  {
      // "Degenerate" segment, return either d1 or d2
      return d1;
  }

  return_type d2 = m_strategy.apply(sp2, p);

  return_type crs_AD = geometry::detail::course<return_type>(sp1, p);
  return_type crs_AB = geometry::detail::course<return_type>(sp1, sp2);
  return_type crs_BA = crs_AB - geometry::math::pi<return_type>();
  return_type crs_BD = geometry::detail::course<return_type>(sp2, p);
  return_type d_crs1 = crs_AD - crs_AB;
  return_type d_crs2 = crs_BD - crs_BA;

  // d1, d2, d3 are in principle not needed, only the sign matters
  return_type projection1 = cos( d_crs1 ) * d1 / d3;
  return_type projection2 = cos( d_crs2 ) * d2 / d3;

  if (projection1 > 0.0 && projection2 > 0.0)
  {
      return_type XTD
          = radius() * math::abs( asin( sin( d1 / radius() ) * sin( d_crs1 ) ));

      // Return shortest distance, projected point on segment sp1-sp2
      return return_type(XTD);
  }
  else
  {
      // Return shortest distance, project either on point sp1 or sp2
      return return_type( (std::min)( d1 , d2 ) );
  }

  ================= reference implementation -- end =================


  Motivation
  ----------
  In what follows we develop a comparable version of the cross track
  distance strategy, that meets the following goals:
  * It is more efficient than the original cross track strategy (less
    operations and less calls to mathematical functions).
  * Distances using the comparable cross track strategy can not only
    be compared with other distances using the same strategy, but also with
    distances computed with the comparable version of the haversine strategy.
  * It can serve as the basis for the computation of the cross track distance,
    as it is more efficient to compute its comparable version and
    transform that to the actual cross track distance, rather than
    follow/use the reference implementation listed above.

  Major idea
  ----------
  The idea here is to use the comparable haversine strategy to compute
  the distances d1, d2 and d3 in the above listing. Once we have done
  that we need also to make sure that instead of returning XTD (as
  computed above) that we return a distance CXTD that is compatible
  with the comparable haversine distance. To achieve this CXTD must satisfy
  the relation:
      XTD = 2 * R * asin( sqrt(XTD) )
  where R is the sphere's radius.

  Below we perform the mathematical analysis that show how to compute CXTD.


  Mathematical analysis
  ---------------------
  Below we use the following trigonometric identities:
      sin(2 * x) = 2 * sin(x) * cos(x)
      cos(asin(x)) = sqrt(1 - x^2)

  Observation:
  The distance d1 needed when the projection of the point D is within the
  segment must be the true distance. However, comparable::haversine<>
  returns a comparable distance instead of the one needed.
  To remedy this, we implicitly compute what is needed. 
  More precisely, we need to compute sin(true_d1):

  sin(true_d1) = sin(2 * asin(sqrt(d1)))
               = 2 * sin(asin(sqrt(d1)) * cos(asin(sqrt(d1)))
               = 2 * sqrt(d1) * sqrt(1-(sqrt(d1))^2)
               = 2 * sqrt(d1 - d1 * d1)
  This relation is used below.

  As we mentioned above the goal is to find CXTD (named "a" below for
  brevity) such that ("b" below stands for "d1", and "c" for "d_crs1"):

      2 * R * asin(sqrt(a)) == R * asin(2 * sqrt(b-b^2) * sin(c))

  Analysis:
      2 * R * asin(sqrt(a)) == R * asin(2 * sqrt(b-b^2) * sin(c))
  <=> 2 * asin(sqrt(a)) == asin(sqrt(b-b^2) * sin(c))
  <=> sin(2 * asin(sqrt(a))) == 2 * sqrt(b-b^2) * sin(c)
  <=> 2 * sin(asin(sqrt(a))) * cos(asin(sqrt(a))) == 2 * sqrt(b-b^2) * sin(c)
  <=> 2 * sqrt(a) * sqrt(1-a) == 2 * sqrt(b-b^2) * sin(c)
  <=> sqrt(a) * sqrt(1-a) == sqrt(b-b^2) * sin(c)
  <=> sqrt(a-a^2) == sqrt(b-b^2) * sin(c)
  <=> a-a^2 == (b-b^2) * (sin(c))^2

  Consider the quadratic equation: x^2-x+p^2 == 0,
  where p = sqrt(b-b^2) * sin(c); its discriminant is:
      d = 1 - 4 * p^2 = 1 - 4 * (b-b^2) * (sin(c))^2

  The two solutions are:
      a_1 = (1 - sqrt(d)) / 2
      a_2 = (1 + sqrt(d)) / 2

  Which one to choose?
  "a" refers to the distance (on the unit sphere) of D from the
  supporting great circle Circ(A,B) of the segment AB.
  The two different values for "a" correspond to the lengths of the two
  arcs delimited D and the points of intersection of Circ(A,B) and the
  great circle perperdicular to Circ(A,B) passing through D.
  Clearly, the value we want is the smallest among these two distances,
  hence the root we must choose is the smallest root among the two.

  So the answer is:
      CXTD = ( 1 - sqrt(1 - 4 * (b-b^2) * (sin(c))^2) ) / 2

  Therefore, in order to implement the comparable version of the cross
  track strategy we need to:
  (1) Use the comparable version of the haversine strategy instead of
      the non-comparable one.
  (2) Instead of return XTD when D projects inside the segment AB, we
      need to return CXTD, given by the following formula:
          CXTD = ( 1 - sqrt(1 - 4 * (d1-d1^2) * (sin(d_crs1))^2) ) / 2;


  Complexity Analysis
  -------------------
  In the analysis that follows we refer to the actual implementation below.
  In particular, instead of computing CXTD as above, we use the more
  efficient (operation-wise) computation of CXTD shown here:

      return_type sin_d_crs1 = sin(d_crs1);
      return_type d1_x_sin = d1 * sin_d_crs1;
      return_type d = d1_x_sin * (sin_d_crs1 - d1_x_sin);
      return d / (0.5 + math::sqrt(0.25 - d));

  Notice that instead of computing:
      0.5 - 0.5 * sqrt(1 - 4 * d) = 0.5 - sqrt(0.25 - d)
  we use the following formula instead:
      d / (0.5 + sqrt(0.25 - d)).
  This is done for numerical robustness. The expression 0.5 - sqrt(0.25 - x)
  has large numerical errors for values of x close to 0 (if using doubles
  the error start to become large even when d is as large as 0.001).
  To remedy that, we re-write 0.5 - sqrt(0.25 - x) as:
      0.5 - sqrt(0.25 - d)
      = (0.5 - sqrt(0.25 - d) * (0.5 - sqrt(0.25 - d)) / (0.5 + sqrt(0.25 - d)).
  The numerator is the difference of two squares:
      (0.5 - sqrt(0.25 - d) * (0.5 - sqrt(0.25 - d))
      = 0.5^2 - (sqrt(0.25 - d))^ = 0.25 - (0.25 - d) = d,
  which gives the expression we use.

  For the complexity analysis, we distinguish between two cases:
  (A) The distance is realized between the point D and an
      endpoint of the segment AB

      Gains:
      Since we are using comparable::haversine<> which is called
      3 times, we gain:
      -> 3 calls to sqrt
      -> 3 calls to asin
      -> 6 multiplications

      Loses: None

      So the net gain is:
      -> 6 function calls (sqrt/asin)
      -> 6 arithmetic operations

      If we use comparable::cross_track<> to compute
      cross_track<> we need to account for a call to sqrt, a call
      to asin and 2 multiplications. In this case the net gain is:
      -> 4 function calls (sqrt/asin)
      -> 4 arithmetic operations


  (B) The distance is realized between the point D and an
      interior point of the segment AB

      Gains:
      Since we are using comparable::haversine<> which is called
      3 times, we gain:
      -> 3 calls to sqrt
      -> 3 calls to asin
      -> 6 multiplications
      Also we gain the operations used to compute XTD:
      -> 2 calls to sin
      -> 1 call to asin
      -> 1 call to abs
      -> 2 multiplications
      -> 1 division
      So the total gains are:
      -> 9 calls to sqrt/sin/asin
      -> 1 call to abs
      -> 8 multiplications
      -> 1 division

      Loses:
      To compute a distance compatible with comparable::haversine<>
      we need to perform a few more operations, namely:
      -> 1 call to sin
      -> 1 call to sqrt
      -> 2 multiplications
      -> 1 division
      -> 1 addition
      -> 2 subtractions

      So roughly speaking the net gain is:
      -> 8 fewer function calls and 3 fewer arithmetic operations

      If we were to implement cross_track directly from the
      comparable version (much like what haversine<> does using
      comparable::haversine<>) we need additionally
      -> 2 function calls (asin/sqrt)
      -> 2 multiplications

      So it pays off to re-implement cross_track<> to use
      comparable::cross_track<>; in this case the net gain would be:
      -> 6 function calls
      -> 1 arithmetic operation

   Summary/Conclusion
   ------------------
   Following the mathematical and complexity analysis above, the
   comparable cross track strategy (as implemented below) satisfies
   all the goal mentioned in the beginning:
   * It is more efficient than its non-comparable counter-part.
   * Comparable distances using this new strategy can also be compared
     with comparable distances computed with the comparable haversine
     strategy.
   * It turns out to be more efficient to compute the actual cross
     track distance XTD by first computing CXTD, and then computing
     XTD by means of the formula:
                XTD = 2 * R * asin( sqrt(CXTD) )
*/

template
<
    typename CalculationType = void,
    typename Strategy = comparable::haversine<double, CalculationType>
>
class cross_track
{
public:
    template <typename Point, typename PointOfSegment>
    struct return_type
        : promote_floating_point
          <
              typename select_calculation_type
                  <
                      Point,
                      PointOfSegment,
                      CalculationType
                  >::type
          >
    {};

    typedef typename Strategy::radius_type radius_type;

    cross_track() = default;

    explicit inline cross_track(typename Strategy::radius_type const& r)
        : m_strategy(r)
    {}

    inline cross_track(Strategy const& s)
        : m_strategy(s)
    {}

    // It might be useful in the future
    // to overload constructor with strategy info.
    // crosstrack(...) {}


    template <typename Point, typename PointOfSegment>
    inline typename return_type<Point, PointOfSegment>::type
    apply(Point const& p, PointOfSegment const& sp1, PointOfSegment const& sp2) const
    {

#if !defined(BOOST_MSVC)
        BOOST_CONCEPT_ASSERT
            (
                (concepts::PointDistanceStrategy<Strategy, Point, PointOfSegment>)
            );
#endif

        typedef typename return_type<Point, PointOfSegment>::type return_type;

        // http://williams.best.vwh.net/avform.htm#XTE
        return_type d1 = m_strategy.apply(sp1, p);
        return_type d3 = m_strategy.apply(sp1, sp2);

        if (geometry::math::equals(d3, 0.0))
        {
            // "Degenerate" segment, return either d1 or d2
            return d1;
        }

        return_type d2 = m_strategy.apply(sp2, p);

        return_type lon1 = geometry::get_as_radian<0>(sp1);
        return_type lat1 = geometry::get_as_radian<1>(sp1);
        return_type lon2 = geometry::get_as_radian<0>(sp2);
        return_type lat2 = geometry::get_as_radian<1>(sp2);
        return_type lon = geometry::get_as_radian<0>(p);
        return_type lat = geometry::get_as_radian<1>(p);

        return_type crs_AD = geometry::formula::spherical_azimuth<return_type, false>
                             (lon1, lat1, lon, lat).azimuth;

        geometry::formula::result_spherical<return_type> result =
                geometry::formula::spherical_azimuth<return_type, true>
                    (lon1, lat1, lon2, lat2);
        return_type crs_AB = result.azimuth;
        return_type crs_BA = result.reverse_azimuth - geometry::math::pi<return_type>();

        return_type crs_BD = geometry::formula::spherical_azimuth<return_type, false>
                             (lon2, lat2, lon, lat).azimuth;

        return_type d_crs1 = crs_AD - crs_AB;
        return_type d_crs2 = crs_BD - crs_BA;

        // d1, d2, d3 are in principle not needed, only the sign matters
        return_type projection1 = cos( d_crs1 ) * d1 / d3;
        return_type projection2 = cos( d_crs2 ) * d2 / d3;

#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
        std::cout << "Course " << dsv(sp1) << " to " << dsv(p) << " "
                  << crs_AD * geometry::math::r2d<return_type>() << std::endl;
        std::cout << "Course " << dsv(sp1) << " to " << dsv(sp2) << " "
                  << crs_AB * geometry::math::r2d<return_type>() << std::endl;
        std::cout << "Course " << dsv(sp2) << " to " << dsv(sp1) << " "
                  << crs_BA * geometry::math::r2d<return_type>() << std::endl;
        std::cout << "Course " << dsv(sp2) << " to " << dsv(p) << " "
                  << crs_BD * geometry::math::r2d<return_type>() << std::endl;
        std::cout << "Projection AD-AB " << projection1 << " : "
                  << d_crs1 * geometry::math::r2d<return_type>() << std::endl;
        std::cout << "Projection BD-BA " << projection2 << " : "
                  << d_crs2 * geometry::math::r2d<return_type>() << std::endl;
        std::cout << " d1: " << (d1 )
                  << " d2: " << (d2 )
                  << std::endl;
#endif

        if (projection1 > 0.0 && projection2 > 0.0)
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
            return_type XTD = radius() * geometry::math::abs( asin( sin( d1 ) * sin( d_crs1 ) ));

            std::cout << "Projection ON the segment" << std::endl;
            std::cout << "XTD: " << XTD
                      << " d1: " << (d1 * radius())
                      << " d2: " << (d2 * radius())
                      << std::endl;
#endif
            return_type const half(0.5);
            return_type const quarter(0.25);

            return_type sin_d_crs1 = sin(d_crs1);
            /*
              This is the straightforward obvious way to continue:
              
              return_type discriminant
                  = 1.0 - 4.0 * (d1 - d1 * d1) * sin_d_crs1 * sin_d_crs1;
              return 0.5 - 0.5 * math::sqrt(discriminant);
            
              Below we optimize the number of arithmetic operations
              and account for numerical robustness:
            */
            return_type d1_x_sin = d1 * sin_d_crs1;
            return_type d = d1_x_sin * (sin_d_crs1 - d1_x_sin);
            return d / (half + math::sqrt(quarter - d));
        }
        else
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
            std::cout << "Projection OUTSIDE the segment" << std::endl;
#endif

            // Return shortest distance, project either on point sp1 or sp2
            return return_type( (std::min)( d1 , d2 ) );
        }
    }

    template <typename T1, typename T2>
    inline radius_type vertical_or_meridian(T1 lat1, T2 lat2) const
    {
        return m_strategy.radius() * (lat1 - lat2);
    }

    inline typename Strategy::radius_type radius() const
    { return m_strategy.radius(); }

private :
    Strategy m_strategy;
};

} // namespace comparable


/*!
\brief Strategy functor for distance point to segment calculation
\ingroup strategies
\details Class which calculates the distance of a point to a segment, for points on a sphere or globe
\see http://williams.best.vwh.net/avform.htm
\tparam CalculationType \tparam_calculation
\tparam Strategy underlying point-point distance strategy, defaults to haversine

\qbk{
[heading See also]
[link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
}

*/
template
<
    typename CalculationType = void,
    typename Strategy = haversine<double, CalculationType>
>
class cross_track
{
public :
    typedef within::spherical_point_point equals_point_point_strategy_type;

    typedef intersection::spherical_segments
        <
            CalculationType
        > relate_segment_segment_strategy_type;

    static inline relate_segment_segment_strategy_type get_relate_segment_segment_strategy()
    {
        return relate_segment_segment_strategy_type();
    }

    typedef within::spherical_winding
        <
            void, void, CalculationType
        > point_in_geometry_strategy_type;

    static inline point_in_geometry_strategy_type get_point_in_geometry_strategy()
    {
        return point_in_geometry_strategy_type();
    }

    template <typename Point, typename PointOfSegment>
    struct return_type
        : promote_floating_point
          <
              typename select_calculation_type
                  <
                      Point,
                      PointOfSegment,
                      CalculationType
                  >::type
          >
    {};

    typedef typename Strategy::radius_type radius_type;

    inline cross_track()
    {}

    explicit inline cross_track(typename Strategy::radius_type const& r)
        : m_strategy(r)
    {}

    inline cross_track(Strategy const& s)
        : m_strategy(s)
    {}

    // It might be useful in the future
    // to overload constructor with strategy info.
    // crosstrack(...) {}


    template <typename Point, typename PointOfSegment>
    inline typename return_type<Point, PointOfSegment>::type
    apply(Point const& p, PointOfSegment const& sp1, PointOfSegment const& sp2) const
    {

#if !defined(BOOST_MSVC)
        BOOST_CONCEPT_ASSERT
            (
                (concepts::PointDistanceStrategy<Strategy, Point, PointOfSegment>)
            );
#endif
        typedef typename return_type<Point, PointOfSegment>::type return_type;
        typedef cross_track<CalculationType, Strategy> this_type;

        typedef typename services::comparable_type
            <
                this_type
            >::type comparable_type;

        comparable_type cstrategy
            = services::get_comparable<this_type>::apply(m_strategy);

        return_type const a = cstrategy.apply(p, sp1, sp2);
        return_type const c = return_type(2.0) * asin(math::sqrt(a));
        return c * radius();
    }

    template <typename T1, typename T2>
    inline radius_type vertical_or_meridian(T1 lat1, T2 lat2) const
    {
        return m_strategy.radius() * (lat1 - lat2);
    }

    inline typename Strategy::radius_type radius() const
    { return m_strategy.radius(); }

private :

    Strategy m_strategy;
};



#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType, typename Strategy>
struct tag<cross_track<CalculationType, Strategy> >
{
    typedef strategy_tag_distance_point_segment type;
};


template <typename CalculationType, typename Strategy, typename P, typename PS>
struct return_type<cross_track<CalculationType, Strategy>, P, PS>
    : cross_track<CalculationType, Strategy>::template return_type<P, PS>
{};


template <typename CalculationType, typename Strategy>
struct comparable_type<cross_track<CalculationType, Strategy> >
{
    typedef comparable::cross_track
        <
            CalculationType, typename comparable_type<Strategy>::type
        >  type;
};


template
<
    typename CalculationType,
    typename Strategy
>
struct get_comparable<cross_track<CalculationType, Strategy> >
{
    typedef typename comparable_type
        <
            cross_track<CalculationType, Strategy>
        >::type comparable_type;
public :
    static inline comparable_type
    apply(cross_track<CalculationType, Strategy> const& strategy)
    {
        return comparable_type(strategy.radius());
    }
};


template
<
    typename CalculationType,
    typename Strategy,
    typename P,
    typename PS
>
struct result_from_distance<cross_track<CalculationType, Strategy>, P, PS>
{
private :
    typedef typename cross_track
        <
            CalculationType, Strategy
        >::template return_type<P, PS>::type return_type;
public :
    template <typename T>
    static inline return_type
    apply(cross_track<CalculationType, Strategy> const& , T const& distance)
    {
        return distance;
    }
};


// Specializations for comparable::cross_track
template <typename RadiusType, typename CalculationType>
struct tag<comparable::cross_track<RadiusType, CalculationType> >
{
    typedef strategy_tag_distance_point_segment type;
};


template
<
    typename RadiusType,
    typename CalculationType,
    typename P,
    typename PS
>
struct return_type<comparable::cross_track<RadiusType, CalculationType>, P, PS>
    : comparable::cross_track
        <
            RadiusType, CalculationType
        >::template return_type<P, PS>
{};


template <typename RadiusType, typename CalculationType>
struct comparable_type<comparable::cross_track<RadiusType, CalculationType> >
{
    typedef comparable::cross_track<RadiusType, CalculationType> type;
};


template <typename RadiusType, typename CalculationType>
struct get_comparable<comparable::cross_track<RadiusType, CalculationType> >
{
private :
    typedef comparable::cross_track<RadiusType, CalculationType> this_type;
public :
    static inline this_type apply(this_type const& input)
    {
        return input;
    }
};


template
<
    typename RadiusType,
    typename CalculationType,
    typename P,
    typename PS
>
struct result_from_distance
    <
        comparable::cross_track<RadiusType, CalculationType>, P, PS
    >
{
private :
    typedef comparable::cross_track<RadiusType, CalculationType> strategy_type;
    typedef typename return_type<strategy_type, P, PS>::type return_type;
public :
    template <typename T>
    static inline return_type apply(strategy_type const& strategy,
                                    T const& distance)
    {
        return_type const s
            = sin( (distance / strategy.radius()) / return_type(2.0) );
        return s * s;
    }
};



/*

TODO:  spherical polar coordinate system requires "get_as_radian_equatorial<>"

template <typename Point, typename PointOfSegment, typename Strategy>
struct default_strategy
    <
        segment_tag, Point, PointOfSegment,
        spherical_polar_tag, spherical_polar_tag,
        Strategy
    >
{
    typedef cross_track
        <
            void,
            std::conditional_t
                <
                    std::is_void<Strategy>::value,
                    typename default_strategy
                        <
                            point_tag, Point, PointOfSegment,
                            spherical_polar_tag, spherical_polar_tag
                        >::type,
                    Strategy
                >
        > type;
};
*/

template <typename Point, typename PointOfSegment, typename Strategy>
struct default_strategy
    <
        point_tag, segment_tag, Point, PointOfSegment,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef cross_track
        <
            void,
            std::conditional_t
                <
                    std::is_void<Strategy>::value,
                    typename default_strategy
                        <
                            point_tag, point_tag, Point, PointOfSegment,
                            spherical_equatorial_tag, spherical_equatorial_tag
                        >::type,
                    Strategy
                >
        > type;
};


template <typename PointOfSegment, typename Point, typename Strategy>
struct default_strategy
    <
        segment_tag, point_tag, PointOfSegment, Point,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef typename default_strategy
        <
            point_tag, segment_tag, Point, PointOfSegment,
            spherical_equatorial_tag, spherical_equatorial_tag,
            Strategy
        >::type type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

}} // namespace strategy::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_HPP
