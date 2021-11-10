// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_TRANSFORM_VARIANT_HPP
#define BOOST_GEOMETRY_UTIL_TRANSFORM_VARIANT_HPP


#include <boost/config/pragma_message.hpp>
#if !defined(BOOST_ALLOW_DEPRECATED_HEADERS)
BOOST_PRAGMA_MESSAGE("This header is deprecated.")
#endif


#include <boost/mpl/transform.hpp>
#include <boost/variant/variant_fwd.hpp>


namespace boost { namespace geometry
{


/*!
    \brief Meta-function that takes a Sequence type, an MPL lambda
        expression and an optional Inserter and returns a variant type over
        the same types as the initial variant type, each transformed using
        the lambda expression.
    \ingroup utility
    \par Example
    \code
        typedef boost::mpl::vector<int, float, long> types;
        typedef transform_variant<types, add_pointer<_> > transformed;
        typedef variant<int*, float*, long*> result;
        BOOST_MPL_ASSERT(( equal<result, transformed> ));
    \endcode
*/
template <typename Sequence, typename Op, typename In = boost::mpl::na>
struct transform_variant:
    make_variant_over<
        typename boost::mpl::transform<
            Sequence,
            Op,
            In
        >::type
    >
{};


/*!
    \brief Meta-function that takes a boost::variant type and an MPL lambda
        expression and returns a variant type over the same types as the
        initial variant type, each transformed using the lambda expression.
    \ingroup utility
    \par Example
    \code
        typedef variant<int, float, long> variant_type;
        typedef transform_variant<variant_type, add_pointer<_> > transformed;
        typedef variant<int*, float*, long*> result;
        BOOST_MPL_ASSERT(( equal<result, transformed> ));
    \endcode
*/
template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Op>
struct transform_variant<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Op, boost::mpl::na> :
    make_variant_over<
        typename boost::mpl::transform<
            typename variant<BOOST_VARIANT_ENUM_PARAMS(T)>::types,
            Op
        >::type
    >
{};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_UTIL_TRANSFORM_VARIANT_HPP
