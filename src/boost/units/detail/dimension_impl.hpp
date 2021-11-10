// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DIMENSION_IMPL_HPP
#define BOOST_UNITS_DIMENSION_IMPL_HPP

#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/less.hpp>

#include <boost/units/config.hpp>
#include <boost/units/dimensionless_type.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/units_fwd.hpp>
#include <boost/units/detail/dimension_list.hpp>
#include <boost/units/detail/push_front_if.hpp>
#include <boost/units/detail/push_front_or_add.hpp>

/// \file 
/// \brief Core class and metaprogramming utilities for compile-time dimensional analysis.

namespace boost {

namespace units {

namespace detail {

template<int N>
struct insertion_sort_dims_insert;

template<bool is_greater>
struct insertion_sort_dims_comparison_impl;

// have to recursively add the element to the next sequence.
template<>
struct insertion_sort_dims_comparison_impl<true> {
    template<class Begin, int N, class T>
    struct apply {
        typedef list<
            typename Begin::item,
            typename insertion_sort_dims_insert<N - 1>::template apply<
                typename Begin::next,
                T
            >::type
        > type;
    };
};

// either prepend the current element or join it to
// the first remaining element of the sequence.
template<>
struct insertion_sort_dims_comparison_impl<false> {
    template<class Begin, int N, class T>
    struct apply {
        typedef typename push_front_or_add<Begin, T>::type type;
    };
};

template<int N>
struct insertion_sort_dims_insert {
    template<class Begin, class T>
    struct apply {
        typedef typename insertion_sort_dims_comparison_impl<mpl::less<typename Begin::item, T>::value>::template apply<
            Begin,
            N,
            T
        >::type type;
    };
};

template<>
struct insertion_sort_dims_insert<0> {
    template<class Begin, class T>
    struct apply {
        typedef list<T, dimensionless_type> type;
    };
};

template<int N>
struct insertion_sort_dims_mpl_sequence {
    template<class Begin>
    struct apply {
        typedef typename insertion_sort_dims_mpl_sequence<N - 1>::template apply<typename mpl::next<Begin>::type>::type next;
        typedef typename insertion_sort_dims_insert<(next::size::value)>::template apply<next, typename mpl::deref<Begin>::type>::type type;
    };
};

template<>
struct insertion_sort_dims_mpl_sequence<0> {
    template<class Begin>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<int N>
struct insertion_sort_dims_impl {
    template<class Begin>
    struct apply {
        typedef typename insertion_sort_dims_impl<N - 1>::template apply<typename Begin::next>::type next;
        typedef typename insertion_sort_dims_insert<(next::size::value)>::template apply<next, typename Begin::item>::type type;
    };
};

template<>
struct insertion_sort_dims_impl<0> {
    template<class Begin>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<class T>
struct sort_dims
{
    typedef typename insertion_sort_dims_mpl_sequence<mpl::size<T>::value>::template apply<typename mpl::begin<T>::type>::type type;
};


template<class T, class Next>
struct sort_dims<list<T, Next> >
{
    typedef typename insertion_sort_dims_impl<list<T, Next>::size::value>::template apply<list<T, Next> >::type type;
};

/// sorted sequences can be merged in linear time
template<bool less, bool greater>
struct merge_dimensions_func;

template<int N1, int N2>
struct merge_dimensions_impl;

template<>
struct merge_dimensions_func<true, false>
{
    template<typename Begin1, typename Begin2, int N1, int N2>
    struct apply
    {
        typedef list<
            typename Begin1::item,
            typename merge_dimensions_impl<N1 - 1, N2>::template apply<
                typename Begin1::next,
                Begin2
            >::type
        > type;
    };
};

template<>
struct merge_dimensions_func<false, true> {
    template<typename Begin1, typename Begin2, int N1, int N2>
    struct apply
    {
        typedef list<
            typename Begin2::item,
            typename merge_dimensions_impl<N2 - 1, N1>::template apply<
                typename Begin2::next,
                Begin1
            >::type
        > type;
    };
};

template<>
struct merge_dimensions_func<false, false> {
    template<typename Begin1, typename Begin2, int N1, int N2>
    struct apply
    {
        typedef typename mpl::plus<typename Begin1::item, typename Begin2::item>::type combined;
        typedef typename push_front_if<!is_empty_dim<combined>::value>::template apply<
            typename merge_dimensions_impl<N1 - 1, N2 - 1>::template apply<
                typename Begin1::next,
                typename Begin2::next
            >::type,
            combined
        >::type type;
    };
};

template<int N1, int N2>
struct merge_dimensions_impl {
    template<typename Begin1, typename Begin2>
    struct apply
    {
        typedef typename Begin1::item dim1;
        typedef typename Begin2::item dim2;

        typedef typename merge_dimensions_func<(mpl::less<dim1,dim2>::value == true),
                (mpl::less<dim2,dim1>::value == true)>::template apply<
            Begin1,
            Begin2,
            N1,
            N2
        >::type type;
    };
};

template<typename Sequence1, typename Sequence2>
struct merge_dimensions
{
    typedef typename detail::merge_dimensions_impl<Sequence1::size::value, 
                                                   Sequence2::size::value>::template 
        apply<
            Sequence1,
            Sequence2
        >::type type;
};

template<int N>
struct iterator_to_list
{
    template<typename Begin>
    struct apply
    {
        typedef list<
            typename Begin::item,
            typename iterator_to_list<N - 1>::template apply<
                typename Begin::next
            >::type
        > type;
    };
};

template<>
struct iterator_to_list<0>
{
    template<typename Begin>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<int N>
struct merge_dimensions_impl<N, 0>
{
    template<typename Begin1, typename Begin2>
    struct apply
    {
        typedef typename iterator_to_list<N>::template apply<Begin1>::type type;
    };
};

template<int N>
struct merge_dimensions_impl<0, N>
{
    template<typename Begin1, typename Begin2>
    struct apply
    {
        typedef typename iterator_to_list<N>::template apply<Begin2>::type type;
    };
};

template<>
struct merge_dimensions_impl<0, 0>
{
    template<typename Begin1, typename Begin2>
    struct apply
    {
        typedef dimensionless_type type;
    };
};

template<int N>
struct static_inverse_impl
{
    template<typename Begin>
    struct apply {
        typedef list<
            typename mpl::negate<typename Begin::item>::type,
            typename static_inverse_impl<N - 1>::template apply<
                typename Begin::next
            >::type
        > type;
    };
};

template<>
struct static_inverse_impl<0>
{
    template<typename Begin>
    struct apply
    {
        typedef dimensionless_type type;
    };
};

template<int N>
struct static_power_impl
{
    template<typename Begin, typename Ex>
    struct apply
    {
        typedef list<
            typename mpl::times<typename Begin::item, Ex>::type,
            typename detail::static_power_impl<N - 1>::template apply<typename Begin::next, Ex>::type
        > type;
    };
};

template<>
struct static_power_impl<0>
{
    template<typename Begin, typename Ex>
    struct apply
    {
        typedef dimensionless_type type;
    };
};

template<int N>
struct static_root_impl {
    template<class Begin, class Ex>
    struct apply {
        typedef list<
            typename mpl::divides<typename Begin::item, Ex>::type,
            typename detail::static_root_impl<N - 1>::template apply<typename Begin::next, Ex>::type
        > type;
    };
};

template<>
struct static_root_impl<0> {
    template<class Begin, class Ex>
    struct apply 
    {
        typedef dimensionless_type type;
    };
};

} // namespace detail

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_DIMENSION_IMPL_HPP
