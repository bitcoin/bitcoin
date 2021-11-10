// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.

// This file was modified by Oracle on 2016-2020.
// Modifications copyright (c) 2016-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ARITHMETIC_CROSS_PRODUCT_HPP
#define BOOST_GEOMETRY_ARITHMETIC_CROSS_PRODUCT_HPP


#include <cstddef>
#include <type_traits>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/make.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <std::size_t Dimension>
struct cross_product
{
    // We define cross product only for 2d (see Wolfram) and 3d.
    // In Math, it is also well-defined for 7-dimension.
    // Generalisation of cross product to n-dimension is defined as
    // wedge product but it is not direct analogue to binary cross product.
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Dimension.",
        std::integral_constant<std::size_t, Dimension>);
};

template <>
struct cross_product<2>
{
    template <typename P1, typename P2, typename ResultP>
    static void apply(P1 const& p1, P2 const& p2, ResultP& result)
    {
        assert_dimension<P1, 2>();
        assert_dimension<P2, 2>();
        assert_dimension<ResultP, 2>();

        // For 2-dimensions, analog of the cross product U(x,y) and V(x,y) is
        // Ux * Vy - Uy * Vx
        // which is returned as 0-component (or X) of 2d vector, 1-component is undefined.
        set<0>(result, get<0>(p1) * get<1>(p2) - get<1>(p1) * get<0>(p2));
    }
};

template <>
struct cross_product<3>
{
    template <typename P1, typename P2, typename ResultP>
    static void apply(P1 const& p1, P2 const& p2, ResultP& result)
    {
        assert_dimension<P1, 3>();
        assert_dimension<P2, 3>();
        assert_dimension<ResultP, 3>();

        set<0>(result, get<1>(p1) * get<2>(p2) - get<2>(p1) * get<1>(p2));
        set<1>(result, get<2>(p1) * get<0>(p2) - get<0>(p1) * get<2>(p2));
        set<2>(result, get<0>(p1) * get<1>(p2) - get<1>(p1) * get<0>(p2));
    }

    template <typename ResultP, typename P1, typename P2>
    static constexpr ResultP apply(P1 const& p1, P2 const& p2)
    {
        assert_dimension<P1, 3>();
        assert_dimension<P2, 3>();
        assert_dimension<ResultP, 3>();

        return traits::make<ResultP>::apply(
                get<1>(p1) * get<2>(p2) - get<2>(p1) * get<1>(p2),
                get<2>(p1) * get<0>(p2) - get<0>(p1) * get<2>(p2),
                get<0>(p1) * get<1>(p2) - get<1>(p1) * get<0>(p2));
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Computes the cross product of two vectors.
\details All vectors should have the same dimension, 3 or 2.
\ingroup arithmetic
\param p1 first vector
\param p2 second vector
\return the cross product vector

*/

template
<
    typename ResultP, typename P1, typename P2,
    std::enable_if_t
        <
            dimension<ResultP>::value != 3
         || ! traits::make<ResultP>::is_specialized,
            int
        > = 0
>
inline ResultP cross_product(P1 const& p1, P2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<ResultP>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<P1>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<P2>) );

    ResultP result;
    detail::cross_product<dimension<ResultP>::value>::apply(p1, p2, result);
    return result;
}

template
<
    typename ResultP, typename P1, typename P2,
    std::enable_if_t
        <
            dimension<ResultP>::value == 3
         && traits::make<ResultP>::is_specialized,
            int
        > = 0
>
// workaround for VS2015
#if !defined(_MSC_VER) || (_MSC_VER >= 1910)
constexpr
#endif
inline ResultP cross_product(P1 const& p1, P2 const& p2)
{
    BOOST_CONCEPT_ASSERT((concepts::Point<ResultP>));
    BOOST_CONCEPT_ASSERT((concepts::ConstPoint<P1>));
    BOOST_CONCEPT_ASSERT((concepts::ConstPoint<P2>));

    return detail::cross_product<3>::apply<ResultP>(p1, p2);
}

/*!
\brief Computes the cross product of two vectors.
\details All vectors should have the same dimension, 3 or 2.
\ingroup arithmetic
\param p1 first vector
\param p2 second vector
\return the cross product vector

\qbk{[heading Examples]}
\qbk{[cross_product] [cross_product_output]}
*/
template
<
    typename P,
    std::enable_if_t
        <
            dimension<P>::value != 3
         || ! traits::make<P>::is_specialized,
            int
        > = 0
>
inline P cross_product(P const& p1, P const& p2)
{
    BOOST_CONCEPT_ASSERT((concepts::Point<P>));
    BOOST_CONCEPT_ASSERT((concepts::ConstPoint<P>));

    P result;
    detail::cross_product<dimension<P>::value>::apply(p1, p2, result);
    return result;
}


template
<
    typename P,
    std::enable_if_t
        <
            dimension<P>::value == 3
         && traits::make<P>::is_specialized,
            int
        > = 0
>
// workaround for VS2015
#if !defined(_MSC_VER) || (_MSC_VER >= 1910)
constexpr
#endif
inline P cross_product(P const& p1, P const& p2)
{
    BOOST_CONCEPT_ASSERT((concepts::Point<P>));
    BOOST_CONCEPT_ASSERT((concepts::ConstPoint<P>));

    return detail::cross_product<3>::apply<P>(p1, p2);
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ARITHMETIC_CROSS_PRODUCT_HPP
