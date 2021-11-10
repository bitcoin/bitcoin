// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DIMENSION_HPP
#define BOOST_UNITS_DIMENSION_HPP

#include <boost/static_assert.hpp>

#include <boost/type_traits/is_same.hpp>

#include <boost/mpl/arithmetic.hpp>

#include <boost/units/static_rational.hpp>
#include <boost/units/detail/dimension_list.hpp>
#include <boost/units/detail/dimension_impl.hpp>

/// \file 
/// \brief Core metaprogramming utilities for compile-time dimensional analysis.

namespace boost {

namespace units {

/// Reduce dimension list to cardinal form. This algorithm collapses duplicate
/// base dimension tags and sorts the resulting list by the tag ordinal value.
/// Dimension lists that resolve to the same dimension are guaranteed to be  
/// represented by an identical type.
///
/// The argument should be an MPL forward sequence containing instances
/// of the @c dim template.
///
/// The result is also an MPL forward sequence.  It also supports the
/// following metafunctions to allow use as a dimension.
///
///    - @c mpl::plus is defined only on two equal dimensions and returns the argument unchanged.
///    - @c mpl::minus is defined only for two equal dimensions and returns the argument unchanged.
///    - @c mpl::negate will return its argument unchanged.
///    - @c mpl::times is defined for any dimensions and adds corresponding exponents.
///    - @c mpl::divides is defined for any dimensions and subtracts the exponents of the
///         right had argument from the corresponding exponents of the left had argument.
///         Missing base dimension tags are assumed to have an exponent of zero.
///    - @c static_power takes a dimension and a static_rational and multiplies all
///         the exponents of the dimension by the static_rational.
///    - @c static_root takes a dimension and a static_rational and divides all
///         the exponents of the dimension by the static_rational.
template<typename Seq>
struct make_dimension_list
{
    typedef typename detail::sort_dims<Seq>::type type;
};

/// Raise a dimension list to a scalar power.
template<typename DL,typename Ex> 
struct static_power
{
    typedef typename detail::static_power_impl<DL::size::value>::template apply<
        DL,
        Ex
    >::type type;    
};

/// Take a scalar root of a dimension list.
template<typename DL,typename Rt> 
struct static_root
{
    typedef typename detail::static_root_impl<DL::size::value>::template apply<
        DL,
        Rt
    >::type type;    
};

} // namespace units

#ifndef BOOST_UNITS_DOXYGEN

namespace mpl {

template<>
struct plus_impl<boost::units::detail::dimension_list_tag,boost::units::detail::dimension_list_tag>
{
    template<class T0, class T1>
    struct apply
    {
        BOOST_STATIC_ASSERT((boost::is_same<T0,T1>::value == true));
        typedef T0 type;
    };
};

template<>
struct minus_impl<boost::units::detail::dimension_list_tag,boost::units::detail::dimension_list_tag>
{
    template<class T0, class T1>
    struct apply
    {
        BOOST_STATIC_ASSERT((boost::is_same<T0,T1>::value == true));
        typedef T0 type;
    };
};

template<>
struct times_impl<boost::units::detail::dimension_list_tag,boost::units::detail::dimension_list_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef typename boost::units::detail::merge_dimensions<T0,T1>::type type;
    };
};

template<>
struct divides_impl<boost::units::detail::dimension_list_tag,boost::units::detail::dimension_list_tag>
{
    template<class T0, class T1>
    struct apply
    {
        typedef typename boost::units::detail::merge_dimensions<
            T0,
            typename boost::units::detail::static_inverse_impl<
                T1::size::value
            >::template apply<
                T1
            >::type
        >::type type;
    };
};

template<>
struct negate_impl<boost::units::detail::dimension_list_tag>
{
    template<class T0>
    struct apply
    {
        typedef T0 type;
    };
};

} // namespace mpl

#endif

} // namespace boost

#endif // BOOST_UNITS_DIMENSION_HPP
