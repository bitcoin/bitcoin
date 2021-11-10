// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_SORT_HPP
#define BOOST_UNITS_DETAIL_SORT_HPP

#include <boost/mpl/size.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/less.hpp>

#include <boost/units/dimensionless_type.hpp>
#include <boost/units/detail/dimension_list.hpp>

namespace boost {

namespace units {

namespace detail {

template<int N>
struct insertion_sort_insert;

template<bool is_greater>
struct insertion_sort_comparison_impl;

// have to recursively add the element to the next sequence.
template<>
struct insertion_sort_comparison_impl<true> {
    template<class Begin, int N, class T>
    struct apply {
        typedef list<
            typename Begin::item,
            typename insertion_sort_insert<N - 1>::template apply<
                typename Begin::next,
                T
            >::type
        > type;
    };
};

// prepend the current element
template<>
struct insertion_sort_comparison_impl<false> {
    template<class Begin, int N, class T>
    struct apply {
        typedef list<T, Begin> type;
    };
};

template<int N>
struct insertion_sort_insert {
    template<class Begin, class T>
    struct apply {
        typedef typename insertion_sort_comparison_impl<mpl::less<typename Begin::item, T>::value>::template apply<
            Begin,
            N,
            T
        >::type type;
    };
};

template<>
struct insertion_sort_insert<0> {
    template<class Begin, class T>
    struct apply {
        typedef list<T, dimensionless_type> type;
    };
};

template<int N>
struct insertion_sort_impl {
    template<class Begin>
    struct apply {
        typedef typename insertion_sort_impl<N - 1>::template apply<typename Begin::next>::type next;
        typedef typename insertion_sort_insert<(next::size::value)>::template apply<next, typename Begin::item>::type type;
    };
};

template<>
struct insertion_sort_impl<0> {
    template<class Begin>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<class T>
struct insertion_sort
{
    typedef typename insertion_sort_impl<T::size::value>::template apply<T>::type type;
};

} // namespace detail

} // namespace units

} // namespace boost

#endif
