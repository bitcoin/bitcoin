// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_ABSOLUTE_IMPL_HPP
#define BOOST_UNITS_ABSOLUTE_IMPL_HPP

#include <iosfwd>

#include <boost/units/config.hpp>
#include <boost/units/conversion.hpp>
#include <boost/units/heterogeneous_system.hpp>
#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

/// INTERNAL ONLY
template<class D, class S>
struct reduce_unit<absolute<unit<D, S> > >
{
    typedef absolute<typename reduce_unit<unit<D, S> >::type> type;
};

namespace detail {

struct undefined_affine_conversion_base {
    BOOST_STATIC_CONSTEXPR bool is_defined = false;
};

} // namespace detail

/// INTERNAL ONLY
template<class From, class To>
struct affine_conversion_helper : detail::undefined_affine_conversion_base { };

namespace detail {

template<bool IsDefined, bool ReverseIsDefined>
struct affine_conversion_impl;

template<bool ReverseIsDefined>
struct affine_conversion_impl<true, ReverseIsDefined>
{
    template<class Unit1, class Unit2, class T0, class T1>
    struct apply {
        static BOOST_CONSTEXPR T1 value(const T0& t0)
        {
            return(
                t0 * 
                conversion_factor(Unit1(), Unit2()) +
                affine_conversion_helper<typename reduce_unit<Unit1>::type, typename reduce_unit<Unit2>::type>::value());
        }
    };
};

template<>
struct affine_conversion_impl<false, true>
{
    template<class Unit1, class Unit2, class T0, class T1>
    struct apply
    {
        static BOOST_CONSTEXPR T1 value(const T0& t0)
        {
            return(
                (t0 - affine_conversion_helper<typename reduce_unit<Unit2>::type, typename reduce_unit<Unit1>::type>::value()) * 
                conversion_factor(Unit1(), Unit2()));
        }
    };
};

} // namespace detail

/// INTERNAL ONLY
template<class Unit1, class T1, class Unit2, class T2>
struct conversion_helper<quantity<absolute<Unit1>, T1>, quantity<absolute<Unit2>, T2> >
{
    typedef quantity<absolute<Unit1>, T1> from_quantity_type;
    typedef quantity<absolute<Unit2>, T2> to_quantity_type;
    static BOOST_CONSTEXPR to_quantity_type convert(const from_quantity_type& source)
    {
        return(
            to_quantity_type::from_value(
                detail::affine_conversion_impl<
                    affine_conversion_helper<typename reduce_unit<Unit1>::type, typename reduce_unit<Unit2>::type>::is_defined,
                    affine_conversion_helper<typename reduce_unit<Unit2>::type, typename reduce_unit<Unit1>::type>::is_defined
                >::template apply<Unit1, Unit2, T1, T2>::value(source.value())
            )
        );
    }
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_ABSOLUTE_IMPL_HPP
