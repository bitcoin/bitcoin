// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_DIMENSIONLESS_UNIT_HPP
#define BOOST_UNITS_DETAIL_DIMENSIONLESS_UNIT_HPP

#include <boost/mpl/bool.hpp>
#include <boost/units/units_fwd.hpp>

namespace boost {
namespace units {

template<class T>
struct heterogeneous_system;

template<class T>
struct homogeneous_system;

template<class T1, class T2, class Scale>
struct heterogeneous_system_impl;

typedef boost::units::heterogeneous_system<
    boost::units::heterogeneous_system_impl<
        boost::units::dimensionless_type,
        boost::units::dimensionless_type,
        boost::units::dimensionless_type
    >
> heterogeneous_dimensionless_system;

namespace detail {

template<class System>
struct void_if_dimensionless {
    typedef int type;
};

template<class T>
struct void_if_dimensionless<boost::units::homogeneous_system<T> > {
    typedef void type;
};

template<>
struct void_if_dimensionless<heterogeneous_dimensionless_system> {
    typedef void type;
};

template<class System, class Test = void>
struct void_if_heterogeneous {
    typedef void type;
};

template<class System>
struct void_if_heterogeneous<System, typename void_if_dimensionless<System>::type> {
    typedef int type;
};

template<class System, class Enable=void>
struct is_dimensionless_system : mpl::false_ {};

template<class System>
struct is_dimensionless_system<System, typename void_if_dimensionless<System>::type> : mpl::true_ {};

#define BOOST_UNITS_DIMENSIONLESS_UNIT(T)\
    boost::units::unit<\
        boost::units::dimensionless_type,\
        T,\
        typename ::boost::units::detail::void_if_dimensionless<T>::type\
    >

#define BOOST_UNITS_HETEROGENEOUS_DIMENSIONLESS_UNIT(T)\
    boost::units::unit<\
        boost::units::dimensionless_type,\
        T,\
        typename ::boost::units::detail::void_if_heterogeneous<T>::type\
    >

}
}
}

#endif
