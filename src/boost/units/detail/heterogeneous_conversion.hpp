// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_HETEROGENEOUS_CONVERSION_HPP
#define BOOST_UNITS_DETAIL_HETEROGENEOUS_CONVERSION_HPP

#include <boost/mpl/minus.hpp>
#include <boost/mpl/times.hpp>

#include <boost/units/static_rational.hpp>
#include <boost/units/homogeneous_system.hpp>
#include <boost/units/detail/linear_algebra.hpp>

namespace boost {

namespace units {

namespace detail {

struct solve_end {
    template<class Begin, class Y>
    struct apply {
        typedef dimensionless_type type;
    };
};

struct no_solution {};

template<class X1, class X2, class Next>
struct solve_normal {
    template<class Begin, class Y>
    struct apply {
        typedef typename Begin::next next;
        typedef list<
            typename mpl::minus<
                typename mpl::times<X1, Y>::type,
                typename mpl::times<X2, typename Begin::item>::type
            >::type,
            typename Next::template apply<next, Y>::type
        > type;
    };
};

template<class Next>
struct solve_leading_zeroes {
    template<class Begin>
    struct apply {
        typedef list<
            typename Begin::item,
            typename Next::template apply<typename Begin::next>::type
        > type;
    };
    typedef solve_leading_zeroes type;
};

template<>
struct solve_leading_zeroes<no_solution> {
    typedef no_solution type;
};

template<class Next>
struct solve_first_non_zero {
    template<class Begin>
    struct apply {
        typedef typename Next::template apply<
            typename Begin::next,
            typename Begin::item
        >::type type;
    };
};

template<class Next>
struct solve_internal_zero {
    template<class Begin, class Y>
    struct apply {
        typedef list<
            typename Begin::item,
            typename Next::template apply<typename Begin::next, Y>::type
        > type;
    };
};

template<class T>
struct make_solve_list_internal_zero {
    template<class Next, class X>
    struct apply {
        typedef solve_normal<T, X, Next> type;
    };
};

template<>
struct make_solve_list_internal_zero<static_rational<0> > {
    template<class Next, class X>
    struct apply {
        typedef solve_internal_zero<Next> type;
    };
};

template<int N>
struct make_solve_list_normal {
    template<class Begin, class X>
    struct apply {
        typedef typename make_solve_list_internal_zero<
            typename Begin::item
        >::template apply<
            typename make_solve_list_normal<N-1>::template apply<typename Begin::next, X>::type,
            X
        >::type type;
    };
};

template<>
struct make_solve_list_normal<0> {
    template<class Begin, class X>
    struct apply {
        typedef solve_end type;
    };
};

template<int N>
struct make_solve_list_leading_zeroes;

template<class T>
struct make_solve_list_first_non_zero {
    template<class Begin, int N>
    struct apply {
        typedef solve_first_non_zero<
            typename make_solve_list_normal<N-1>::template apply<
                typename Begin::next,
                typename Begin::item
            >::type
        > type;
    };
};

template<>
struct make_solve_list_first_non_zero<static_rational<0> > {
    template<class Begin, int N>
    struct apply {
        typedef typename solve_leading_zeroes<
            typename make_solve_list_leading_zeroes<N-1>::template apply<
                typename Begin::next
            >::type
        >::type type;
    };
};

template<int N>
struct make_solve_list_leading_zeroes {
    template<class Begin>
    struct apply {
        typedef typename make_solve_list_first_non_zero<typename Begin::item>::template apply<Begin, N>::type type;
    };
};

template<>
struct make_solve_list_leading_zeroes<0> {
    template<class Begin>
    struct apply {
        typedef no_solution type;
    };
};

template<int N>
struct try_add_unit_impl {
    template<class Begin, class L>
    struct apply {
        typedef typename try_add_unit_impl<N-1>::template apply<typename Begin::next, L>::type next;
        typedef typename Begin::item::template apply<next>::type type;
        BOOST_STATIC_ASSERT((next::size::value - 1 == type::size::value));
    };
};

template<>
struct try_add_unit_impl<0> {
    template<class Begin, class L>
    struct apply {
        typedef L type;
    };
};

template<int N>
struct make_homogeneous_system_impl;

template<class T, bool is_done>
struct make_homogeneous_system_func;

template<class T>
struct make_homogeneous_system_func<T, false> {
    template<class Begin, class Current, class Units, class Dimensions, int N>
    struct apply {
        typedef typename make_homogeneous_system_impl<N-1>::template apply<
            typename Begin::next,
            list<T, Current>,
            list<typename Begin::item, Units>,
            Dimensions
        >::type type;
    };
};

template<class T>
struct make_homogeneous_system_func<T, true> {
    template<class Begin, class Current, class Units, class Dimensions, int N>
    struct apply {
        typedef list<typename Begin::item, Units> type;
    };
};

template<>
struct make_homogeneous_system_func<no_solution, false> {
    template<class Begin, class Current, class Units, class Dimensions, int N>
    struct apply {
        typedef typename make_homogeneous_system_impl<N-1>::template apply<
            typename Begin::next,
            Current,
            Units,
            Dimensions
        >::type type;
    };
};

template<>
struct make_homogeneous_system_func<no_solution, true> {
    template<class Begin, class Current, class Units, class Dimensions, int N>
    struct apply {
        typedef typename make_homogeneous_system_impl<N-1>::template apply<
            typename Begin::next,
            Current,
            Units,
            Dimensions
        >::type type;
    };
};

template<int N>
struct make_homogeneous_system_impl {
    template<class Begin, class Current, class Units, class Dimensions>
    struct apply {
        typedef typename expand_dimensions<Dimensions::size::value>::template apply<
            Dimensions,
            typename Begin::item::dimension_type
        >::type dimensions;
        typedef typename try_add_unit_impl<Current::size::value>::template apply<Current, dimensions>::type new_element;
        typedef typename make_solve_list_leading_zeroes<new_element::size::value>::template apply<new_element>::type new_func;
        typedef typename make_homogeneous_system_func<
            new_func,
            ((Current::size::value)+1) == (Dimensions::size::value)
        >::template apply<Begin, Current, Units, Dimensions, N>::type type;
    };
};

template<>
struct make_homogeneous_system_impl<0> {
    template<class Begin, class Current, class Units, class Dimensions>
    struct apply {
        typedef Units type;
    };
};

template<class Units>
struct make_homogeneous_system {
    typedef typename find_base_dimensions<Units>::type base_dimensions;
    typedef homogeneous_system<
        typename insertion_sort<
            typename make_homogeneous_system_impl<
                Units::size::value
            >::template apply<
                Units,
                dimensionless_type,
                dimensionless_type,
                base_dimensions
            >::type
        >::type
    > type;
};

template<int N>
struct extract_base_units {
    template<class Begin, class T>
    struct apply {
        typedef list<
            typename Begin::item::tag_type,
            typename extract_base_units<N-1>::template apply<typename Begin::next, T>::type
        > type;
    };
};

template<>
struct extract_base_units<0> {
    template<class Begin, class T>
    struct apply {
        typedef T type;
    };
};

}

}

}

#endif
