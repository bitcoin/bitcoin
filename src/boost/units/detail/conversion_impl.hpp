// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_CONVERSION_IMPL_HPP
#define BOOST_UNITS_DETAIL_CONVERSION_IMPL_HPP

#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/divides.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/units/heterogeneous_system.hpp>
#include <boost/units/homogeneous_system.hpp>
#include <boost/units/reduce_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/dimension_list.hpp>
#include <boost/units/detail/heterogeneous_conversion.hpp>
#include <boost/units/detail/one.hpp>
#include <boost/units/detail/static_rational_power.hpp>
#include <boost/units/detail/unscale.hpp>

#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

namespace detail {

template<class Source, class Dest>
struct conversion_factor_helper;

template<class Source, class Dest>
struct call_base_unit_converter;

}

/// INTERNAL ONLY
struct undefined_base_unit_converter_base {
    BOOST_STATIC_CONSTEXPR bool is_defined = false;
};

/// INTERNAL ONLY
struct no_default_conversion {
    BOOST_STATIC_CONSTEXPR bool is_defined = false;
};

/// INTERNAL ONLY
template<class BaseUnit>
struct unscaled_get_default_conversion : no_default_conversion { };

/// INTERNAL ONLY
template<bool is_defined>
struct unscaled_get_default_conversion_impl;

/// INTERNAL ONLY
template<>
struct unscaled_get_default_conversion_impl<true>
{
    template<class T>
    struct apply
    {
        typedef typename unscaled_get_default_conversion<typename unscale<T>::type>::type type;
    };
};

/// INTERNAL ONLY
template<>
struct unscaled_get_default_conversion_impl<false>
{
    template<class T>
    struct apply
    {
        typedef typename T::unit_type type;
    };
};

/// INTERNAL ONLY
template<class BaseUnit>
struct get_default_conversion
{
    typedef typename unscaled_get_default_conversion_impl<
        unscaled_get_default_conversion<typename unscale<BaseUnit>::type>::is_defined
    >::template apply<BaseUnit>::type type;
};

/// INTERNAL ONLY
template<class Source, class Destination>
struct select_base_unit_converter
{
    typedef Source source_type;
    typedef Destination destination_type;
};

/// INTERNAL ONLY
template<class Source, class Dest>
struct base_unit_converter_base : undefined_base_unit_converter_base {
};

/// INTERNAL ONLY
template<class Source>
struct base_unit_converter_base<Source, BOOST_UNITS_MAKE_HETEROGENEOUS_UNIT(Source, typename Source::dimension_type)>
{
    BOOST_STATIC_CONSTEXPR bool is_defined = true;
    typedef one type;
    static BOOST_CONSTEXPR type value() {
        return(one());
    }
};

/// INTERNAL ONLY
template<class Source, class Dest>
struct base_unit_converter : base_unit_converter_base<Source, Dest> { };

namespace detail {

template<class Source, class Dest>
struct do_call_base_unit_converter {
    typedef select_base_unit_converter<typename unscale<Source>::type, typename unscale<Dest>::type> selector;
    typedef typename selector::source_type source_type;
    typedef typename selector::destination_type destination_type;
    typedef base_unit_converter<source_type, destination_type> converter;
    typedef typename mpl::divides<typename get_scale_list<Source>::type, typename get_scale_list<source_type>::type>::type source_factor;
    typedef typename mpl::divides<typename get_scale_list<Dest>::type, typename get_scale_list<destination_type>::type>::type destination_factor;
    typedef typename mpl::divides<source_factor, destination_factor>::type factor;
    typedef eval_scale_list<factor> eval_factor;
    typedef typename multiply_typeof_helper<typename converter::type, typename eval_factor::type>::type type;
    static BOOST_CONSTEXPR type value()
    {
        return(converter::value() * eval_factor::value());
    }
};

template<bool forward_is_defined, bool reverse_is_defined>
struct call_base_unit_converter_base_unit_impl;

template<>
struct call_base_unit_converter_base_unit_impl<true, true>
{
    template<class Source, class Dest>
    struct apply
        : do_call_base_unit_converter<Source, typename Dest::unit_type> 
    {
    };
};

template<>
struct call_base_unit_converter_base_unit_impl<true, false>
{
    template<class Source, class Dest>
    struct apply
        : do_call_base_unit_converter<Source, typename Dest::unit_type> 
    {
    };
};

template<>
struct call_base_unit_converter_base_unit_impl<false, true>
{
    template<class Source, class Dest>
    struct apply
    {
        typedef do_call_base_unit_converter<Dest, typename Source::unit_type> converter;
        typedef typename divide_typeof_helper<one, typename converter::type>::type type;
        static BOOST_CONSTEXPR type value() {
            return(one() / converter::value());
        }
    };
};

template<>
struct call_base_unit_converter_base_unit_impl<false, false>
{
    template<class Source, class Dest>
    struct apply
    {
        typedef typename reduce_unit<typename get_default_conversion<Source>::type>::type new_source;
        typedef typename reduce_unit<typename get_default_conversion<Dest>::type>::type new_dest;
        typedef call_base_unit_converter<Source, new_source> start;
        typedef detail::conversion_factor_helper<
            new_source,
            new_dest
        > conversion;
        typedef call_base_unit_converter<Dest, new_dest> end;
        typedef typename divide_typeof_helper<
            typename multiply_typeof_helper<
                typename start::type,
                typename conversion::type
            >::type,
            typename end::type
        >::type type;
        static BOOST_CONSTEXPR type value() {
            return(start::value() * conversion::value() / end::value());
        }
    };
};

template<int N>
struct get_default_conversion_impl
{
    template<class Begin>
    struct apply
    {
        typedef typename Begin::item source_pair;
        typedef typename source_pair::value_type exponent;
        typedef typename source_pair::tag_type source;
        typedef typename reduce_unit<typename get_default_conversion<source>::type>::type new_source;
        typedef typename get_default_conversion_impl<N-1>::template apply<typename Begin::next> next_iteration;
        typedef typename multiply_typeof_helper<typename power_typeof_helper<new_source, exponent>::type, typename next_iteration::unit_type>::type unit_type;
        typedef call_base_unit_converter<source, new_source> conversion;
        typedef typename multiply_typeof_helper<typename conversion::type, typename next_iteration::type>::type type;
        static BOOST_CONSTEXPR type value() {
            return(static_rational_power<exponent>(conversion::value()) * next_iteration::value());
        }
    };
};

template<>
struct get_default_conversion_impl<0>
{
    template<class Begin>
    struct apply
    {
        typedef unit<dimensionless_type, heterogeneous_system<heterogeneous_system_impl<dimensionless_type, dimensionless_type, no_scale> > > unit_type;
        typedef one type;
        static BOOST_CONSTEXPR one value() {
            return(one());
        }
    };
};

template<bool is_defined>
struct call_base_unit_converter_impl;

template<>
struct call_base_unit_converter_impl<true>
{
    template<class Source, class Dest>
    struct apply
        : do_call_base_unit_converter<Source, Dest> 
    {
    };
};

template<>
struct call_base_unit_converter_impl<false>
{
    template<class Source, class Dest>
    struct apply {
        typedef typename reduce_unit<typename get_default_conversion<Source>::type>::type new_source;
        typedef typename Dest::system_type::type system_list;
        typedef typename get_default_conversion_impl<system_list::size::value>::template apply<system_list> impl;
        typedef typename impl::unit_type new_dest;
        typedef call_base_unit_converter<Source, new_source> start;
        typedef conversion_factor_helper<new_source, new_dest> conversion;
        typedef typename divide_typeof_helper<
            typename multiply_typeof_helper<
                typename start::type,
                typename conversion::type
            >::type,
            typename impl::type
        >::type type;
        static BOOST_CONSTEXPR type value() {
            return(start::value() * conversion::value() / impl::value());
        }
    };
};

#define BOOST_UNITS_DETAIL_BASE_UNIT_CONVERTER_IS_DEFINED(Source, Dest)\
    base_unit_converter<\
        typename select_base_unit_converter<typename unscale<Source>::type, typename unscale<Dest>::type>::source_type,\
        typename select_base_unit_converter<typename unscale<Source>::type, typename unscale<Dest>::type>::destination_type\
    >::is_defined

template<class Source, class Dest>
struct call_base_unit_converter : call_base_unit_converter_impl<BOOST_UNITS_DETAIL_BASE_UNIT_CONVERTER_IS_DEFINED(Source, Dest)>::template apply<Source, Dest>
{
};

template<class Source, class Dest>
struct call_base_unit_converter<Source, BOOST_UNITS_MAKE_HETEROGENEOUS_UNIT(Dest, typename Source::dimension_type)> :
    call_base_unit_converter_base_unit_impl<
        BOOST_UNITS_DETAIL_BASE_UNIT_CONVERTER_IS_DEFINED(Source, typename Dest::unit_type),
        BOOST_UNITS_DETAIL_BASE_UNIT_CONVERTER_IS_DEFINED(Dest, typename Source::unit_type)
    >::template apply<Source, Dest>
{
};

template<int N>
struct conversion_impl
{
    template<class Begin, class DestinationSystem>
    struct apply
    {
        typedef typename conversion_impl<N-1>::template apply<
            typename Begin::next,
            DestinationSystem
        > next_iteration;
        typedef typename Begin::item unit_pair;
        typedef typename unit_pair::tag_type unit;
        typedef typename unit::dimension_type dimensions;
        typedef typename reduce_unit<units::unit<dimensions, DestinationSystem> >::type reduced_unit;
        typedef detail::call_base_unit_converter<unit, reduced_unit> converter;
        typedef typename multiply_typeof_helper<typename converter::type, typename next_iteration::type>::type type;
        static BOOST_CONSTEXPR type value() { return(static_rational_power<typename unit_pair::value_type>(converter::value()) * next_iteration::value()); }
    };
};

template<>
struct conversion_impl<0>
{
    template<class Begin, class DestinationSystem>
    struct apply
    {
        typedef one type;
        static BOOST_CONSTEXPR type value() { return(one()); }
    };
};

} // namespace detail

/// forward to conversion_factor (intentionally allowing ADL)
/// INTERNAL ONLY
template<class Unit1, class T1, class Unit2, class T2>
struct conversion_helper<quantity<Unit1, T1>, quantity<Unit2, T2> >
{
    /// INTERNAL ONLY
    typedef quantity<Unit2, T2> destination_type;
    static BOOST_CONSTEXPR destination_type convert(const quantity<Unit1, T1>& source)
    {
        return(destination_type::from_value(static_cast<T2>(source.value() * conversion_factor(Unit1(), Unit2()))));
    }
};

namespace detail {

template<class Source, class Dest>
struct conversion_factor_helper;

template<class D, class L1, class L2>
struct conversion_factor_helper<unit<D, homogeneous_system<L1> >, unit<D, homogeneous_system<L2> > >
  : conversion_factor_helper<
        typename reduce_unit<unit<D, homogeneous_system<L1> > >::type,
        typename reduce_unit<unit<D, homogeneous_system<L2> > >::type
    >
{
    //typedef typename reduce_unit<unit<D, homogeneous_system<L1> > >::type source_unit;
    //typedef typename source_unit::system_type::type unit_list;
    //typedef typename detail::conversion_impl<unit_list::size::value>::template apply<
    //    unit_list,
    //    homogeneous_system<L2>
    //> impl;
    //typedef typename impl::type type;
    //static BOOST_CONSTEXPR type value()
    //{
    //    return(impl::value());
    //}
};

template<class D, class L1, class L2>
struct conversion_factor_helper<unit<D, heterogeneous_system<L1> >, unit<D, homogeneous_system<L2> > >
  : conversion_factor_helper<
        typename reduce_unit<unit<D, heterogeneous_system<L1> > >::type,
        typename reduce_unit<unit<D, homogeneous_system<L2> > >::type
    >
{
    //typedef typename detail::conversion_impl<L1::type::size::value>::template apply<
    //    typename L1::type,
    //    homogeneous_system<L2>
    //> impl;
    //typedef eval_scale_list<typename L1::scale> scale;
    //typedef typename multiply_typeof_helper<typename impl::type, typename scale::type>::type type;
    //static BOOST_CONSTEXPR type value()
    //{
    //    return(impl::value() * scale::value());
    //}
};

// There is no simple algorithm for doing this conversion
// other than just defining it as the reverse of the
// heterogeneous->homogeneous case
template<class D, class L1, class L2>
struct conversion_factor_helper<unit<D, homogeneous_system<L1> >, unit<D, heterogeneous_system<L2> > >
  : conversion_factor_helper<
        typename reduce_unit<unit<D, homogeneous_system<L1> > >::type,
        typename reduce_unit<unit<D, heterogeneous_system<L2> > >::type
    >
{
    //typedef typename detail::conversion_impl<L2::type::size::value>::template apply<
    //    typename L2::type,
    //    homogeneous_system<L1>
    //> impl;
    //typedef eval_scale_list<typename L2::scale> scale;
    //typedef typename multiply_typeof_helper<typename impl::type, typename scale::type>::type type;
    //static BOOST_CONSTEXPR type value()
    //{
    //    return(one() / (impl::value() * scale::value()));
    //}
};

/// Requires that all possible conversions
/// between base units are defined.
template<class D, class S1, class S2>
struct conversion_factor_helper<unit<D, heterogeneous_system<S1> >, unit<D, heterogeneous_system<S2> > >
{
    /// INTERNAL ONLY
    typedef typename detail::extract_base_units<S1::type::size::value>::template apply<
        typename S1::type,
        dimensionless_type
    >::type from_base_units;
    /// INTERNAL ONLY
    typedef typename detail::extract_base_units<S2::type::size::value>::template apply<
        typename S2::type,
        from_base_units
    >::type all_base_units;
    /// INTERNAL ONLY
    typedef typename detail::make_homogeneous_system<all_base_units>::type system;
    typedef typename detail::conversion_impl<S1::type::size::value>::template apply<
        typename S1::type,
        system
    > conversion1;
    typedef typename detail::conversion_impl<S2::type::size::value>::template apply<
        typename S2::type,
        system
    > conversion2;
    typedef eval_scale_list<typename mpl::divides<typename S1::scale, typename S2::scale>::type> scale;
    typedef typename multiply_typeof_helper<
        typename conversion1::type,
        typename divide_typeof_helper<typename scale::type, typename conversion2::type>::type
    >::type type;
    static BOOST_CONSTEXPR type value()
    {
        return(conversion1::value() * (scale::value() / conversion2::value()));
    }
};

} // namespace detail

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CONVERSION_IMPL_HPP
