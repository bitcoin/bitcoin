// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_PUSH_FRONT_OR_ADD_HPP
#define BOOST_UNITS_DETAIL_PUSH_FRONT_OR_ADD_HPP

#include <boost/mpl/plus.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/push_front_if.hpp>

namespace boost {

namespace units {

template<class Item, class Next>
struct list;

namespace detail {

template<class T>
struct is_empty_dim;

/// add an instantiation of dim to Sequence.
template<bool>
struct push_front_or_add_impl;

template<>
struct push_front_or_add_impl<true>
{
    template<typename Sequence, typename T>
    struct apply
    {
        typedef typename mpl::plus<T, typename Sequence::item>::type item;
        typedef typename push_front_if<!is_empty_dim<item>::value>::template apply<
            typename Sequence::next,
            item
        > type;
    };
};

template<>
struct push_front_or_add_impl<false>
{
    template<typename Sequence, typename T>
    struct apply
    {
        typedef list<T, Sequence> type;
    };
};

template<typename Sequence, typename T>
struct push_front_or_add
{
    typedef typename push_front_or_add_impl<boost::is_same<typename T::tag_type, typename Sequence::item::tag_type>::value>::template apply<
        Sequence,
        T
    >::type type;
};

template<typename T>
struct push_front_or_add<dimensionless_type, T>
{
    typedef list<T, dimensionless_type> type;
};

} // namespace detail

} // namespace units

} // namespace boost

#endif
