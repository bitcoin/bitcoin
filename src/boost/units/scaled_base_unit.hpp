// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_SCALED_BASE_UNIT_HPP_INCLUDED
#define BOOST_UNITS_SCALED_BASE_UNIT_HPP_INCLUDED

#include <string>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/less.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/units/config.hpp>
#include <boost/units/dimension.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

template<class T>
struct heterogeneous_system;

template<class T, class D, class Scale>
struct heterogeneous_system_impl;

template<class T, class E>
struct heterogeneous_system_dim;

template<class T>
struct base_unit_info;

/// INTERNAL ONLY
struct scaled_base_unit_tag {};

template<class S, class Scale>
struct scaled_base_unit
{
    /// INTERNAL ONLY
    typedef void boost_units_is_base_unit_type;
    typedef scaled_base_unit type;
    typedef scaled_base_unit_tag tag;
    typedef S system_type;
    typedef Scale scale_type;
    typedef typename S::dimension_type dimension_type;

#ifdef BOOST_UNITS_DOXYGEN

    typedef detail::unspecified unit_type;

#else

    typedef unit<
        dimension_type,
        heterogeneous_system<
            heterogeneous_system_impl<
                list<
                    heterogeneous_system_dim<scaled_base_unit,static_rational<1> >,
                    dimensionless_type
                >,
                dimension_type,
                dimensionless_type
            >
        >
    > unit_type;

#endif

    static std::string symbol()
    {
        return(Scale::symbol() + base_unit_info<S>::symbol());
    }
    static std::string name()
    {
        return(Scale::name() + base_unit_info<S>::name());
    }
};

} // namespace units

} // namespace boost

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::scaled_base_unit, (class)(class))

#endif

namespace boost {

#ifndef BOOST_UNITS_DOXYGEN

namespace mpl {

/// INTERNAL ONLY
template<class Tag>
struct less_impl<boost::units::scaled_base_unit_tag, Tag>
{
    template<class T0, class T1>
    struct apply : mpl::bool_<
        mpl::less<typename T0::system_type, T1>::value ||
    (boost::is_same<typename T0::system_type, T1>::value && ((T0::scale_type::exponent::Numerator) < 0)) > {};
};

/// INTERNAL ONLY
template<class Tag>
struct less_impl<Tag, boost::units::scaled_base_unit_tag>
{
    template<class T0, class T1>
    struct apply : mpl::bool_<
        mpl::less<T0, typename T1::system_type>::value ||
    (boost::is_same<T0, typename T1::system_type>::value && ((T1::scale_type::exponent::Numerator) > 0)) > {};
};

/// INTERNAL ONLY
template<>
struct less_impl<boost::units::scaled_base_unit_tag, boost::units::scaled_base_unit_tag>
{
    template<class T0, class T1>
    struct apply : mpl::bool_<
        mpl::less<typename T0::system_type, typename T1::system_type>::value ||
    ((boost::is_same<typename T0::system_type, typename T1::system_type>::value) &&
     ((T0::scale_type::base) < (T1::scale_type::base) ||
      ((T0::scale_type::base) == (T1::scale_type::base) &&
       mpl::less<typename T0::scale_type::exponent, typename T1::scale_type::exponent>::value))) > {};
};

} // namespace mpl

#endif

} // namespace boost

#endif
