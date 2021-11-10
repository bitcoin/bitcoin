// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_UNSCALE_HPP_INCLUDED
#define BOOST_UNITS_DETAIL_UNSCALE_HPP_INCLUDED

#include <string>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/times.hpp>
#include <boost/mpl/negate.hpp>
#include <boost/mpl/less.hpp>

#include <boost/units/config.hpp>
#include <boost/units/dimension.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/one.hpp>

namespace boost {

namespace units {

template<class T>
struct heterogeneous_system;

template<class T, class D, class Scale>
struct heterogeneous_system_impl;

template<class T, class E>
struct heterogeneous_system_dim;

template<class S, class Scale>
struct scaled_base_unit;

/// removes all scaling from a unit or a base unit.
template<class T>
struct unscale
{
#ifndef BOOST_UNITS_DOXYGEN
    typedef T type;
#else
    typedef detail::unspecified type;
#endif
};

/// INTERNAL ONLY
template<class S, class Scale>
struct unscale<scaled_base_unit<S, Scale> >
{
    typedef typename unscale<S>::type type;
};

/// INTERNAL ONLY
template<class D, class S>
struct unscale<unit<D, S> >
{
    typedef unit<D, typename unscale<S>::type> type;
};

/// INTERNAL ONLY
template<class Scale>
struct scale_list_dim;

/// INTERNAL ONLY
template<class T>
struct get_scale_list
{
    typedef dimensionless_type type;
};

/// INTERNAL ONLY
template<class S, class Scale>
struct get_scale_list<scaled_base_unit<S, Scale> >
{
    typedef typename mpl::times<list<scale_list_dim<Scale>, dimensionless_type>, typename get_scale_list<S>::type>::type type;
};

/// INTERNAL ONLY
template<class D, class S>
struct get_scale_list<unit<D, S> >
{
    typedef typename get_scale_list<S>::type type;
};

/// INTERNAL ONLY
struct scale_dim_tag {};

/// INTERNAL ONLY
template<class Scale>
struct scale_list_dim : Scale
{
    typedef scale_dim_tag tag;
    typedef scale_list_dim type;
};

} // namespace units

#ifndef BOOST_UNITS_DOXYGEN

namespace mpl {

/// INTERNAL ONLY
template<>
struct less_impl<boost::units::scale_dim_tag, boost::units::scale_dim_tag>
{
    template<class T0, class T1>
    struct apply : mpl::bool_<((T0::base) < (T1::base))> {};
};

}

#endif

namespace units {

namespace detail {

template<class Scale>
struct is_empty_dim<scale_list_dim<Scale> > : mpl::false_ {};

template<long N>
struct is_empty_dim<scale_list_dim<scale<N, static_rational<0, 1> > > > : mpl::true_ {};

template<int N>
struct eval_scale_list_impl
{
    template<class Begin>
    struct apply
    {
        typedef typename eval_scale_list_impl<N-1>::template apply<typename Begin::next> next_iteration;
        typedef typename multiply_typeof_helper<typename next_iteration::type, typename Begin::item::value_type>::type type;
        static BOOST_CONSTEXPR type value()
        {
            return(next_iteration::value() * Begin::item::value());
        }
    };
};

template<>
struct eval_scale_list_impl<0>
{
    template<class Begin>
    struct apply
    {
        typedef one type;
        static BOOST_CONSTEXPR one value()
        {
            return(one());
        }
    };
};

}

/// INTERNAL ONLY
template<class T>
struct eval_scale_list : detail::eval_scale_list_impl<T::size::value>::template apply<T> {};

} // namespace units

#ifndef BOOST_UNITS_DOXYGEN

namespace mpl {

/// INTERNAL ONLY
template<>
struct plus_impl<boost::units::scale_dim_tag, boost::units::scale_dim_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef boost::units::scale_list_dim<
            boost::units::scale<
                (T0::base),
                typename mpl::plus<typename T0::exponent, typename T1::exponent>::type
            >
        > type;
    };
};

/// INTERNAL ONLY
template<>
struct negate_impl<boost::units::scale_dim_tag>
{
    template<class T0>
    struct apply
    {
        typedef boost::units::scale_list_dim<
            boost::units::scale<
                (T0::base),
                typename mpl::negate<typename T0::exponent>::type
            >
        > type;
    };
};

/// INTERNAL ONLY
template<>
struct times_impl<boost::units::scale_dim_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef boost::units::scale_list_dim<
            boost::units::scale<
                (T0::base),
                typename mpl::times<typename T0::exponent, T1>::type
            >
        > type;
    };
};

/// INTERNAL ONLY
template<>
struct divides_impl<boost::units::scale_dim_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef boost::units::scale_list_dim<
            boost::units::scale<
                (T0::base),
                typename mpl::divides<typename T0::exponent, T1>::type
            >
        > type;
    };
};

} // namespace mpl

#endif

} // namespace boost

#endif
