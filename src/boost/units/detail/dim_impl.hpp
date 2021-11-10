// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DIM_IMPL_HPP
#define BOOST_UNITS_DIM_IMPL_HPP

#include <boost/mpl/bool.hpp>
#include <boost/mpl/less.hpp>

#include <boost/units/units_fwd.hpp>

/// \file 
/// \brief Class encapsulating a dimension tag/value pair

namespace boost {

namespace units {

namespace detail {

struct dim_tag;

}

}

namespace mpl {

/// Less than comparison for sorting @c dim.
template<>
struct less_impl<boost::units::detail::dim_tag, boost::units::detail::dim_tag>
{
    template<class T0, class T1>
    struct apply : mpl::less<typename T0::tag_type, typename T1::tag_type> {};
};

}

namespace units {

template<class Tag, class Exponent>
struct dim;

template<long N, long D>
class static_rational;

namespace detail {

/// Extract @c tag_type from a @c dim.
template<typename T>
struct get_tag
{
    typedef typename T::tag_type    type;
};

/// Extract @c value_type from a @c dim.
template<typename T>
struct get_value
{
    typedef typename T::value_type    type;
};

/// Determine if a @c dim is empty (has a zero exponent).
template<class T>
struct is_empty_dim;

template<typename T>
struct is_empty_dim< dim<T, static_rational<0, 1> > > :
    mpl::true_ 
{ };

template<typename T, typename V>
struct is_empty_dim< dim<T, V> > :
    mpl::false_ 
{ };

} // namespace detail

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_DIM_IMPL_HPP
