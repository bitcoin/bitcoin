// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_HETEROGENEOUS_SYSTEM_HPP
#define BOOST_UNITS_HETEROGENEOUS_SYSTEM_HPP

/// \file
/// \brief A heterogeneous system is a sorted list of base unit/exponent pairs.

#include <boost/mpl/bool.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/times.hpp>
#include <boost/mpl/divides.hpp>
#include <boost/mpl/negate.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/units/config.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/dimension.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/push_front_if.hpp>
#include <boost/units/detail/push_front_or_add.hpp>
#include <boost/units/detail/linear_algebra.hpp>
#include <boost/units/detail/unscale.hpp>

namespace boost {

namespace units {

namespace detail {

// A normal system is a sorted list of base units.
// A heterogeneous system is a sorted list of base unit/exponent pairs.
// As long as we don't need to convert heterogeneous systems
// directly everything is cool.

template<class T>
struct is_zero : mpl::false_ {};

template<>
struct is_zero<static_rational<0> > : mpl::true_ {};

} // namespace detail

/// INTERNAL ONLY
template<class L, class Dimensions, class Scale>
struct heterogeneous_system_impl
{
    typedef L type;
    typedef Dimensions dimensions;
    typedef Scale scale;
};

/// INTERNAL ONLY
typedef dimensionless_type no_scale;

/// A system that can represent any possible combination
/// of units at the expense of not preserving information
/// about how it was created.  Do not create specializations
/// of this template directly. Instead use @c reduce_unit and
/// @c base_unit<...>::unit_type.
template<class T>
struct heterogeneous_system : T {};

/// INTERNAL ONLY
struct heterogeneous_system_dim_tag {};

/// INTERNAL ONLY
template<class Unit, class Exponent>
struct heterogeneous_system_dim
{
    typedef heterogeneous_system_dim_tag tag;
    typedef heterogeneous_system_dim type;
    typedef Unit tag_type;
    typedef Exponent value_type;
};

/// INTERNAL ONLY
#define BOOST_UNITS_MAKE_HETEROGENEOUS_UNIT(BaseUnit, Dimensions)   \
    boost::units::unit<                                             \
        Dimensions,                                                 \
        boost::units::heterogeneous_system<                         \
            boost::units::heterogeneous_system_impl<                \
                boost::units::list<                       \
                    boost::units::heterogeneous_system_dim<         \
                        BaseUnit,                                   \
                        boost::units::static_rational<1>            \
                    >,                                              \
                    boost::units::dimensionless_type                \
                >,                                                  \
                Dimensions,                                         \
                boost::units::no_scale                              \
            >                                                       \
        >                                                           \
    >

} // namespace units

} // namespace boost


#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::heterogeneous_system_impl, (class)(class)(class))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::heterogeneous_system, (class))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::heterogeneous_system_dim, (class)(class))

#endif

namespace boost {

namespace mpl {

/// INTERNAL ONLY
template<>
struct less_impl<boost::units::heterogeneous_system_dim_tag, boost::units::heterogeneous_system_dim_tag>
{
    template<class T0, class T1>
    struct apply : mpl::less<typename T0::tag_type, typename T1::tag_type> {};
};

}

namespace units {

namespace detail {

template<class Unit1, class Exponent1>
struct is_empty_dim<heterogeneous_system_dim<Unit1,Exponent1> > : detail::is_zero<Exponent1> {};

} // namespace detail

} // namespace units

namespace mpl {

/// INTERNAL ONLY
template<>
struct plus_impl<boost::units::heterogeneous_system_dim_tag, boost::units::heterogeneous_system_dim_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef boost::units::heterogeneous_system_dim<
            typename T0::tag_type,
            typename mpl::plus<typename T0::value_type,typename T1::value_type>::type
        > type;
    };
};

/// INTERNAL ONLY
template<>
struct times_impl<boost::units::heterogeneous_system_dim_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef boost::units::heterogeneous_system_dim<
            typename T0::tag_type,
            typename mpl::times<typename T0::value_type,T1>::type
        > type;
    };
};

/// INTERNAL ONLY
template<>
struct divides_impl<boost::units::heterogeneous_system_dim_tag, boost::units::detail::static_rational_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef boost::units::heterogeneous_system_dim<
            typename T0::tag_type,
            typename mpl::divides<typename T0::value_type,T1>::type
        > type;
    };
};

/// INTERNAL ONLY
template<>
struct negate_impl<boost::units::heterogeneous_system_dim_tag>
{
    template<class T>
    struct apply
    {
        typedef boost::units::heterogeneous_system_dim<typename T::tag_type, typename mpl::negate<typename T::value_type>::type> type;
    };
};

} // namespace mpl

namespace units {

namespace detail {

template<int N>
struct make_heterogeneous_system_impl
{
    template<class UnitsBegin, class ExponentsBegin>
    struct apply
    {
        typedef typename push_front_if<!(is_zero<typename ExponentsBegin::item>::value)>::template apply<
            typename make_heterogeneous_system_impl<N-1>::template apply<
                typename UnitsBegin::next,
                typename ExponentsBegin::next
            >::type,
            heterogeneous_system_dim<typename UnitsBegin::item, typename ExponentsBegin::item>
        >::type type;
    };
};

template<>
struct make_heterogeneous_system_impl<0>
{
    template<class UnitsBegin, class ExponentsBegin>
    struct apply
    {
        typedef dimensionless_type type;
    };
};

template<class Dimensions, class System>
struct make_heterogeneous_system
{
    typedef typename calculate_base_unit_exponents<typename System::type, Dimensions>::type exponents;
    BOOST_MPL_ASSERT_MSG((!boost::is_same<exponents, inconsistent>::value), the_specified_dimension_is_not_representible_in_the_given_system, (types<Dimensions, System>));
    typedef typename make_heterogeneous_system_impl<System::type::size::value>::template apply<
        typename System::type,
        exponents
    >::type unit_list;
    typedef heterogeneous_system<heterogeneous_system_impl<unit_list, Dimensions, no_scale> > type;
};

template<class Dimensions, class T>
struct make_heterogeneous_system<Dimensions, heterogeneous_system<T> >
{
    typedef heterogeneous_system<T> type;
};

template<class T0, class T1>
struct multiply_systems
{
    typedef heterogeneous_system<
        heterogeneous_system_impl<
            typename mpl::times<typename T0::type, typename T1::type>::type,
            typename mpl::times<typename T0::dimensions, typename T1::dimensions>::type,
            typename mpl::times<typename T0::scale, typename T1::scale>::type
        >
    > type;
};

template<class T0, class T1>
struct divide_systems
{
    typedef heterogeneous_system<
        heterogeneous_system_impl<
            typename mpl::divides<typename T0::type, typename T1::type>::type,
            typename mpl::divides<typename T0::dimensions, typename T1::dimensions>::type,
            typename mpl::divides<typename T0::scale, typename T1::scale>::type
        >
    > type;
};

} // namespace detail

/// INTERNAL ONLY
template<class S, long N, long D>
struct static_power<heterogeneous_system<S>, static_rational<N,D> >
{
    typedef heterogeneous_system<
        heterogeneous_system_impl<
            typename static_power<typename S::type, static_rational<N,D> >::type,
            typename static_power<typename S::dimensions, static_rational<N,D> >::type,
            typename static_power<typename S::scale, static_rational<N,D> >::type
        >
    > type;
};

/// INTERNAL ONLY
template<class S, long N, long D>
struct static_root<heterogeneous_system<S>, static_rational<N,D> >
{
    typedef heterogeneous_system<
        heterogeneous_system_impl<
            typename static_root<typename S::type, static_rational<N,D> >::type,
            typename static_root<typename S::dimensions, static_rational<N,D> >::type,
            typename static_root<typename S::scale, static_rational<N,D> >::type
        >
    > type;
};

namespace detail {

template<int N>
struct unscale_heterogeneous_system_impl
{
    template<class Begin>
    struct apply
    {
        typedef typename push_front_or_add<
            typename unscale_heterogeneous_system_impl<N-1>::template apply<
                typename Begin::next
            >::type,
            typename unscale<typename Begin::item>::type
        >::type type;
    };
};

template<>
struct unscale_heterogeneous_system_impl<0>
{
    template<class Begin>
    struct apply
    {
        typedef dimensionless_type type;
    };
};

} // namespace detail

/// Unscale all the base units. e.g
/// km s -> m s
/// cm km -> m^2
/// INTERNAL ONLY
template<class T>
struct unscale<heterogeneous_system<T> >
{
    typedef heterogeneous_system<
        heterogeneous_system_impl<
            typename detail::unscale_heterogeneous_system_impl<
                T::type::size::value
            >::template apply<
                typename T::type
            >::type,
            typename T::dimensions,
            no_scale
        >
    > type;
};

/// INTERNAL ONLY
template<class Unit, class Exponent>
struct unscale<heterogeneous_system_dim<Unit, Exponent> >
{
    typedef heterogeneous_system_dim<typename unscale<Unit>::type, Exponent> type;
};

namespace detail {

template<int N>
struct get_scale_list_of_heterogeneous_system_impl
{
    template<class Begin>
    struct apply
    {
        typedef typename mpl::times<
            typename get_scale_list_of_heterogeneous_system_impl<N-1>::template apply<
                typename Begin::next
            >::type,
            typename get_scale_list<typename Begin::item>::type
        >::type type;
    };
};

template<>
struct get_scale_list_of_heterogeneous_system_impl<0>
{
    template<class Begin>
    struct apply
    {
        typedef dimensionless_type type;
    };
};

} // namespace detail

/// INTERNAL ONLY
template<class T>
struct get_scale_list<heterogeneous_system<T> >
{
    typedef typename mpl::times<
        typename detail::get_scale_list_of_heterogeneous_system_impl<
            T::type::size::value
        >::template apply<typename T::type>::type,
        typename T::scale
    >::type type;
};

/// INTERNAL ONLY
template<class Unit, class Exponent>
struct get_scale_list<heterogeneous_system_dim<Unit, Exponent> >
{
    typedef typename static_power<typename get_scale_list<Unit>::type, Exponent>::type type;
};

namespace detail {

template<class System, class Dimension>
struct check_system : mpl::false_ {};

template<class System, class Dimension, class Scale>
struct check_system<heterogeneous_system<heterogeneous_system_impl<System, Dimension, Scale> >, Dimension> : mpl::true_ {};

} // namespace detail

} // namespace units

} // namespace boost

#endif
