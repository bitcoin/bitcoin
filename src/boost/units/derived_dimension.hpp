// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DERIVED_DIMENSION_HPP
#define BOOST_UNITS_DERIVED_DIMENSION_HPP

#include <boost/units/dim.hpp>
#include <boost/units/dimension.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/dimension_list.hpp>

namespace boost {

namespace units {

/// A utility class for defining composite dimensions with integer powers.
template<class DT1 = dimensionless_type,long E1 = 0,
         class DT2 = dimensionless_type,long E2 = 0,
         class DT3 = dimensionless_type,long E3 = 0,
         class DT4 = dimensionless_type,long E4 = 0,
         class DT5 = dimensionless_type,long E5 = 0,
         class DT6 = dimensionless_type,long E6 = 0,
         class DT7 = dimensionless_type,long E7 = 0,
         class DT8 = dimensionless_type,long E8 = 0>
struct derived_dimension
{
#ifdef BOOST_UNITS_DOXYGEN
    typedef detail::unspecified type;
#else
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >,
                         list< dim< DT2,static_rational<E2> >,
                         list< dim< DT3,static_rational<E3> >,
                         list< dim< DT4,static_rational<E4> >,
                         list< dim< DT5,static_rational<E5> >,
                         list< dim< DT6,static_rational<E6> >,
                         list< dim< DT7,static_rational<E7> >,
                         list< dim< DT8,static_rational<E8> >, dimensionless_type > > > > > > > > >::type type;
#endif
};

/// INTERNAL ONLY
template<class DT1,long E1>
struct derived_dimension<
    DT1, E1,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0>
{
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >, dimensionless_type > >::type type;
};

/// INTERNAL ONLY
template<class DT1,long E1,
         class DT2,long E2>
struct derived_dimension<
    DT1, E1,
    DT2, E2,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0>
{
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >,
                         list< dim< DT2,static_rational<E2> >, dimensionless_type > > >::type type;
};

/// INTERNAL ONLY
template<class DT1,long E1,
         class DT2,long E2,
         class DT3,long E3>
struct derived_dimension<
    DT1, E1,
    DT2, E2,
    DT3, E3,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0>
{
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >,
                         list< dim< DT2,static_rational<E2> >,
                         list< dim< DT3,static_rational<E3> >, dimensionless_type > > > >::type type;
};

/// INTERNAL ONLY
template<class DT1,long E1,
         class DT2,long E2,
         class DT3,long E3,
         class DT4,long E4>
struct derived_dimension<
    DT1, E1,
    DT2, E2,
    DT3, E3,
    DT4, E4,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0>
{
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >,
                         list< dim< DT2,static_rational<E2> >,
                         list< dim< DT3,static_rational<E3> >,
                         list< dim< DT4,static_rational<E4> >, dimensionless_type > > > > >::type type;
};

/// INTERNAL ONLY
template<class DT1,long E1,
         class DT2,long E2,
         class DT3,long E3,
         class DT4,long E4,
         class DT5,long E5>
struct derived_dimension<
    DT1, E1,
    DT2, E2,
    DT3, E3,
    DT4, E4,
    DT5, E5,
    dimensionless_type,0,
    dimensionless_type,0,
    dimensionless_type,0>
{
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >,
                         list< dim< DT2,static_rational<E2> >,
                         list< dim< DT3,static_rational<E3> >,
                         list< dim< DT4,static_rational<E4> >,
                         list< dim< DT5,static_rational<E5> >, dimensionless_type > > > > > >::type type;
};

/// INTERNAL ONLY
template<class DT1,long E1,
         class DT2,long E2,
         class DT3,long E3,
         class DT4,long E4,
         class DT5,long E5,
         class DT6,long E6>
struct derived_dimension<
    DT1, E1,
    DT2, E2,
    DT3, E3,
    DT4, E4,
    DT5, E5,
    DT6, E6,
    dimensionless_type,0,
    dimensionless_type,0>
{
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >,
                         list< dim< DT2,static_rational<E2> >,
                         list< dim< DT3,static_rational<E3> >,
                         list< dim< DT4,static_rational<E4> >,
                         list< dim< DT5,static_rational<E5> >,
                         list< dim< DT6,static_rational<E6> >, dimensionless_type > > > > > > >::type type;
};

/// INTERNAL ONLY
template<class DT1,long E1,
         class DT2,long E2,
         class DT3,long E3,
         class DT4,long E4,
         class DT5,long E5,
         class DT6,long E6,
         class DT7,long E7>
struct derived_dimension<
    DT1, E1,
    DT2, E2,
    DT3, E3,
    DT4, E4,
    DT5, E5,
    DT6, E6,
    DT7, E7,
    dimensionless_type,0>
{
    typedef typename 
    make_dimension_list< list< dim< DT1,static_rational<E1> >,
                         list< dim< DT2,static_rational<E2> >,
                         list< dim< DT3,static_rational<E3> >,
                         list< dim< DT4,static_rational<E4> >,
                         list< dim< DT5,static_rational<E5> >,
                         list< dim< DT6,static_rational<E6> >,
                         list< dim< DT7,static_rational<E7> >, dimensionless_type > > > > > > > >::type type;
};

} // namespace units

} // namespace boost                                                                   

#endif // BOOST_UNITS_DERIVED_DIMENSION_HPP
