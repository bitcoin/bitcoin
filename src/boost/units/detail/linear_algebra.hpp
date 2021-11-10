// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_LINEAR_ALGEBRA_HPP
#define BOOST_UNITS_DETAIL_LINEAR_ALGEBRA_HPP

#include <boost/units/static_rational.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/arithmetic.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/assert.hpp>

#include <boost/units/dim.hpp>
#include <boost/units/dimensionless_type.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/detail/dimension_list.hpp>
#include <boost/units/detail/sort.hpp>

namespace boost {

namespace units {

namespace detail {

// typedef list<rational> equation;

template<int N>
struct eliminate_from_pair_of_equations_impl;

template<class E1, class E2>
struct eliminate_from_pair_of_equations;

template<int N>
struct elimination_impl;

template<bool is_zero, bool element_is_last>
struct elimination_skip_leading_zeros_impl;

template<class Equation, class Vars>
struct substitute;

template<int N>
struct substitute_impl;

template<bool is_end>
struct solve_impl;

template<class T>
struct solve;

template<int N>
struct check_extra_equations_impl;

template<int N>
struct normalize_units_impl;

struct inconsistent {};

// generally useful utilies.

template<int N>
struct divide_equation {
    template<class Begin, class Divisor>
    struct apply {
        typedef list<typename mpl::divides<typename Begin::item, Divisor>::type, typename divide_equation<N - 1>::template apply<typename Begin::next, Divisor>::type> type;
    };
};

template<>
struct divide_equation<0> {
    template<class Begin, class Divisor>
    struct apply {
        typedef dimensionless_type type;
    };
};

// eliminate_from_pair_of_equations takes a pair of
// equations and eliminates the first variable.
//
// equation eliminate_from_pair_of_equations(equation l1, equation l2) {
//     rational x1 = l1.front();
//     rational x2 = l2.front();
//     return(transform(pop_front(l1), pop_front(l2), _1 * x2 - _2 * x1));
// }

template<int N>
struct eliminate_from_pair_of_equations_impl {
    template<class Begin1, class Begin2, class X1, class X2>
    struct apply {
        typedef list<
            typename mpl::minus<
                typename mpl::times<typename Begin1::item, X2>::type,
                typename mpl::times<typename Begin2::item, X1>::type
            >::type,
            typename eliminate_from_pair_of_equations_impl<N - 1>::template apply<
                typename Begin1::next,
                typename Begin2::next,
                X1,
                X2
            >::type
        > type;
    };
};

template<>
struct eliminate_from_pair_of_equations_impl<0> {
    template<class Begin1, class Begin2, class X1, class X2>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<class E1, class E2>
struct eliminate_from_pair_of_equations {
    typedef E1 begin1;
    typedef E2 begin2;
    typedef typename eliminate_from_pair_of_equations_impl<(E1::size::value - 1)>::template apply<
        typename begin1::next,
        typename begin2::next,
        typename begin1::item,
        typename begin2::item
    >::type type;
};



// Stage 1.  Determine which dimensions should
// have dummy base units.  For this purpose
// row reduce the matrix.

template<int N>
struct make_zero_vector {
    typedef list<static_rational<0>, typename make_zero_vector<N - 1>::type> type;
};
template<>
struct make_zero_vector<0> {
    typedef dimensionless_type type;
};

template<int Column, int TotalColumns>
struct create_row_of_identity {
    typedef list<static_rational<0>, typename create_row_of_identity<Column - 1, TotalColumns - 1>::type> type;
};
template<int TotalColumns>
struct create_row_of_identity<0, TotalColumns> {
    typedef list<static_rational<1>, typename make_zero_vector<TotalColumns - 1>::type> type;
};
template<int Column>
struct create_row_of_identity<Column, 0> {
    // error
};

template<int RemainingRows>
struct determine_extra_equations_impl;

template<bool first_is_zero, bool is_last>
struct determine_extra_equations_skip_zeros_impl;

// not the last row and not zero.
template<>
struct determine_extra_equations_skip_zeros_impl<false, false> {
    template<class RowsBegin, int RemainingRows, int CurrentColumn, int TotalColumns, class Result>
    struct apply {
        // remove the equation being eliminated against from the set of equations.
        typedef typename determine_extra_equations_impl<RemainingRows - 1>::template apply<typename RowsBegin::next, typename RowsBegin::item>::type next_equations;
        // since this column was present, strip it out.
        typedef Result type;
    };
};

// the last row but not zero.
template<>
struct determine_extra_equations_skip_zeros_impl<false, true> {
    template<class RowsBegin, int RemainingRows, int CurrentColumn, int TotalColumns, class Result>
    struct apply {
        // remove this equation.
        typedef dimensionless_type next_equations;
        // since this column was present, strip it out.
        typedef Result type;
    };
};


// the first columns is zero but it is not the last column.
// continue with the same loop.
template<>
struct determine_extra_equations_skip_zeros_impl<true, false> {
    template<class RowsBegin, int RemainingRows, int CurrentColumn, int TotalColumns, class Result>
    struct apply {
        typedef typename RowsBegin::next::item next_row;
        typedef typename determine_extra_equations_skip_zeros_impl<
            next_row::item::Numerator == 0,
            RemainingRows == 2  // the next one will be the last.
        >::template apply<
            typename RowsBegin::next,
            RemainingRows - 1,
            CurrentColumn,
            TotalColumns,
            Result
        > next;
        typedef list<typename RowsBegin::item::next, typename next::next_equations> next_equations;
        typedef typename next::type type;
    };
};

// all the elements in this column are zero.
template<>
struct determine_extra_equations_skip_zeros_impl<true, true> {
    template<class RowsBegin, int RemainingRows, int CurrentColumn, int TotalColumns, class Result>
    struct apply {
        typedef list<typename RowsBegin::item::next, dimensionless_type> next_equations;
        typedef list<typename create_row_of_identity<CurrentColumn, TotalColumns>::type, Result> type;
    };
};

template<int RemainingRows>
struct determine_extra_equations_impl {
    template<class RowsBegin, class EliminateAgainst>
    struct apply {
        typedef list<
            typename eliminate_from_pair_of_equations<typename RowsBegin::item, EliminateAgainst>::type,
            typename determine_extra_equations_impl<RemainingRows-1>::template apply<typename RowsBegin::next, EliminateAgainst>::type
        > type;
    };
};

template<>
struct determine_extra_equations_impl<0> {
    template<class RowsBegin, class EliminateAgainst>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<int RemainingColumns, bool is_done>
struct determine_extra_equations {
    template<class RowsBegin, int TotalColumns, class Result>
    struct apply {
        typedef typename RowsBegin::item top_row;
        typedef typename determine_extra_equations_skip_zeros_impl<
            top_row::item::Numerator == 0,
            RowsBegin::size::value == 1
        >::template apply<
            RowsBegin,
            RowsBegin::size::value,
            TotalColumns - RemainingColumns,
            TotalColumns,
            Result
        > column_info;
        typedef typename determine_extra_equations<
            RemainingColumns - 1,
            column_info::next_equations::size::value == 0
        >::template apply<
            typename column_info::next_equations,
            TotalColumns,
            typename column_info::type
        >::type type;
    };
};

template<int RemainingColumns>
struct determine_extra_equations<RemainingColumns, true> {
    template<class RowsBegin, int TotalColumns, class Result>
    struct apply {
        typedef typename determine_extra_equations<RemainingColumns - 1, true>::template apply<
            RowsBegin,
            TotalColumns,
            list<typename create_row_of_identity<TotalColumns - RemainingColumns, TotalColumns>::type, Result>
        >::type type;
    };
};

template<>
struct determine_extra_equations<0, true> {
    template<class RowsBegin, int TotalColumns, class Result>
    struct apply {
        typedef Result type;
    };
};

// Stage 2
// invert the matrix using Gauss-Jordan elimination


template<bool is_zero, bool is_last>
struct invert_strip_leading_zeroes;

template<int N>
struct invert_handle_after_pivot_row;

// When processing column N, none of the first N rows 
// can be the pivot column.
template<int N>
struct invert_handle_inital_rows {
    template<class RowsBegin, class IdentityBegin>
    struct apply {
        typedef typename invert_handle_inital_rows<N - 1>::template apply<
            typename RowsBegin::next,
            typename IdentityBegin::next
        > next;
        typedef typename RowsBegin::item current_row;
        typedef typename IdentityBegin::item current_identity_row;
        typedef typename next::pivot_row pivot_row;
        typedef typename next::identity_pivot_row identity_pivot_row;
        typedef list<
            typename eliminate_from_pair_of_equations_impl<(current_row::size::value) - 1>::template apply<
                typename current_row::next,
                pivot_row,
                typename current_row::item,
                static_rational<1>
            >::type,
            typename next::new_matrix
        > new_matrix;
        typedef list<
            typename eliminate_from_pair_of_equations_impl<(current_identity_row::size::value)>::template apply<
                current_identity_row,
                identity_pivot_row,
                typename current_row::item,
                static_rational<1>
            >::type,
            typename next::identity_result
        > identity_result;
    };
};

// This handles the switch to searching for a pivot column.
// The pivot row will be propagated up in the typedefs
// pivot_row and identity_pivot_row.  It is inserted here.
template<>
struct invert_handle_inital_rows<0> {
    template<class RowsBegin, class IdentityBegin>
    struct apply {
        typedef typename RowsBegin::item current_row;
        typedef typename invert_strip_leading_zeroes<
            (current_row::item::Numerator == 0),
            (RowsBegin::size::value == 1)
        >::template apply<
            RowsBegin,
            IdentityBegin
        > next;
        // results
        typedef list<typename next::pivot_row, typename next::new_matrix> new_matrix;
        typedef list<typename next::identity_pivot_row, typename next::identity_result> identity_result;
        typedef typename next::pivot_row pivot_row;
        typedef typename next::identity_pivot_row identity_pivot_row;
    };
};

// The first internal element which is not zero.
template<>
struct invert_strip_leading_zeroes<false, false> {
    template<class RowsBegin, class IdentityBegin>
    struct apply {
        typedef typename RowsBegin::item current_row;
        typedef typename current_row::item current_value;
        typedef typename divide_equation<(current_row::size::value - 1)>::template apply<typename current_row::next, current_value>::type new_equation;
        typedef typename divide_equation<(IdentityBegin::item::size::value)>::template apply<typename IdentityBegin::item, current_value>::type transformed_identity_equation;
        typedef typename invert_handle_after_pivot_row<(RowsBegin::size::value - 1)>::template apply<
            typename RowsBegin::next,
            typename IdentityBegin::next,
            new_equation,
            transformed_identity_equation
        > next;

        // results
        // Note that we don't add the pivot row to the
        // results here, because it needs to propagated up
        // to the diagonal.
        typedef typename next::new_matrix new_matrix;
        typedef typename next::identity_result identity_result;
        typedef new_equation pivot_row;
        typedef transformed_identity_equation identity_pivot_row;
    };
};

// The one and only non-zero element--at the end
template<>
struct invert_strip_leading_zeroes<false, true> {
    template<class RowsBegin, class IdentityBegin>
    struct apply {
        typedef typename RowsBegin::item current_row;
        typedef typename current_row::item current_value;
        typedef typename divide_equation<(current_row::size::value - 1)>::template apply<typename current_row::next, current_value>::type new_equation;
        typedef typename divide_equation<(IdentityBegin::item::size::value)>::template apply<typename IdentityBegin::item, current_value>::type transformed_identity_equation;

        // results
        // Note that we don't add the pivot row to the
        // results here, because it needs to propagated up
        // to the diagonal.
        typedef dimensionless_type identity_result;
        typedef dimensionless_type new_matrix;
        typedef new_equation pivot_row;
        typedef transformed_identity_equation identity_pivot_row;
    };
};

// One of the initial zeroes
template<>
struct invert_strip_leading_zeroes<true, false> {
    template<class RowsBegin, class IdentityBegin>
    struct apply {
        typedef typename RowsBegin::item current_row;
        typedef typename RowsBegin::next::item next_row;
        typedef typename invert_strip_leading_zeroes<
            next_row::item::Numerator == 0,
            RowsBegin::size::value == 2
        >::template apply<
            typename RowsBegin::next,
            typename IdentityBegin::next
        > next;
        typedef typename IdentityBegin::item current_identity_row;
        // these are propagated up.
        typedef typename next::pivot_row pivot_row;
        typedef typename next::identity_pivot_row identity_pivot_row;
        typedef list<
            typename eliminate_from_pair_of_equations_impl<(current_row::size::value - 1)>::template apply<
                typename current_row::next,
                pivot_row,
                typename current_row::item,
                static_rational<1>
            >::type,
            typename next::new_matrix
        > new_matrix;
        typedef list<
            typename eliminate_from_pair_of_equations_impl<(current_identity_row::size::value)>::template apply<
                current_identity_row,
                identity_pivot_row,
                typename current_row::item,
                static_rational<1>
            >::type,
            typename next::identity_result
        > identity_result;
    };
};

// the last element, and is zero.
// Should never happen.
template<>
struct invert_strip_leading_zeroes<true, true> {
};

template<int N>
struct invert_handle_after_pivot_row {
    template<class RowsBegin, class IdentityBegin, class MatrixPivot, class IdentityPivot>
    struct apply {
        typedef typename invert_handle_after_pivot_row<N - 1>::template apply<
            typename RowsBegin::next,
            typename IdentityBegin::next,
            MatrixPivot,
            IdentityPivot
        > next;
        typedef typename RowsBegin::item current_row;
        typedef typename IdentityBegin::item current_identity_row;
        typedef MatrixPivot pivot_row;
        typedef IdentityPivot identity_pivot_row;

        // results
        typedef list<
            typename eliminate_from_pair_of_equations_impl<(current_row::size::value - 1)>::template apply<
                typename current_row::next,
                pivot_row,
                typename current_row::item,
                static_rational<1>
            >::type,
            typename next::new_matrix
        > new_matrix;
        typedef list<
            typename eliminate_from_pair_of_equations_impl<(current_identity_row::size::value)>::template apply<
                current_identity_row,
                identity_pivot_row,
                typename current_row::item,
                static_rational<1>
            >::type,
            typename next::identity_result
        > identity_result;
    };
};

template<>
struct invert_handle_after_pivot_row<0> {
    template<class RowsBegin, class IdentityBegin, class MatrixPivot, class IdentityPivot>
    struct apply {
        typedef dimensionless_type new_matrix;
        typedef dimensionless_type identity_result;
    };
};

template<int N>
struct invert_impl {
    template<class RowsBegin, class IdentityBegin>
    struct apply {
        typedef typename invert_handle_inital_rows<RowsBegin::size::value - N>::template apply<RowsBegin, IdentityBegin> process_column;
        typedef typename invert_impl<N - 1>::template apply<
            typename process_column::new_matrix,
            typename process_column::identity_result
        >::type type;
    };
};

template<>
struct invert_impl<0> {
    template<class RowsBegin, class IdentityBegin>
    struct apply {
        typedef IdentityBegin type;
    };
};

template<int N>
struct make_identity {
    template<int Size>
    struct apply {
        typedef list<typename create_row_of_identity<Size - N, Size>::type, typename make_identity<N - 1>::template apply<Size>::type> type;
    };
};

template<>
struct make_identity<0> {
    template<int Size>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<class Matrix>
struct make_square_and_invert {
    typedef typename Matrix::item top_row;
    typedef typename determine_extra_equations<(top_row::size::value), false>::template apply<
        Matrix,                 // RowsBegin
        top_row::size::value,   // TotalColumns
        Matrix                  // Result
    >::type invertible;
    typedef typename invert_impl<invertible::size::value>::template apply<
        invertible,
        typename make_identity<invertible::size::value>::template apply<invertible::size::value>::type
    >::type type;
};


// find_base_dimensions takes a list of
// base_units and returns a sorted list
// of all the base_dimensions they use.
//
// list<base_dimension> find_base_dimensions(list<base_unit> l) {
//     set<base_dimension> dimensions;
//     for_each(base_unit unit : l) {
//         for_each(dim d : unit.dimension_type) {
//             dimensions = insert(dimensions, d.tag_type);
//         }
//     }
//     return(sort(dimensions, _1 > _2, front_inserter(list<base_dimension>())));
// }

typedef char set_no;
struct set_yes { set_no dummy[2]; };

template<class T>
struct wrap {};

struct set_end {
    static set_no lookup(...);
    typedef mpl::long_<0> size;
};

template<class T, class Next>
struct set : Next {
    using Next::lookup;
    static set_yes lookup(wrap<T>*);
    typedef T item;
    typedef Next next;
    typedef typename mpl::next<typename Next::size>::type size;
};

template<bool has_key>
struct set_insert;

template<>
struct set_insert<true> {
    template<class Set, class T>
    struct apply {
        typedef Set type;
    };
};

template<>
struct set_insert<false> {
    template<class Set, class T>
    struct apply {
        typedef set<T, Set> type;
    };
};

template<class Set, class T>
struct has_key {
    BOOST_STATIC_CONSTEXPR long size = sizeof(Set::lookup((wrap<T>*)0));
    BOOST_STATIC_CONSTEXPR bool value = (size == sizeof(set_yes));
};

template<int N>
struct find_base_dimensions_impl_impl {
    template<class Begin, class S>
    struct apply {
        typedef typename find_base_dimensions_impl_impl<N-1>::template apply<
            typename Begin::next,
            S
        >::type next;

        typedef typename set_insert<
            (has_key<next, typename Begin::item::tag_type>::value)
        >::template apply<
            next,
            typename Begin::item::tag_type
        >::type type;
    };
};

template<>
struct find_base_dimensions_impl_impl<0> {
    template<class Begin, class S>
    struct apply {
        typedef S type;
    };
};

template<int N>
struct find_base_dimensions_impl {
    template<class Begin>
    struct apply {
        typedef typename find_base_dimensions_impl_impl<(Begin::item::dimension_type::size::value)>::template apply<
            typename Begin::item::dimension_type,
            typename find_base_dimensions_impl<N-1>::template apply<typename Begin::next>::type
        >::type type;
    };
};

template<>
struct find_base_dimensions_impl<0> {
    template<class Begin>
    struct apply {
        typedef set_end type;
    };
};

template<class T>
struct find_base_dimensions {
    typedef typename insertion_sort<
        typename find_base_dimensions_impl<
            (T::size::value)
        >::template apply<T>::type
    >::type type;
};

// calculate_base_dimension_coefficients finds
// the coefficients corresponding to the first
// base_dimension in each of the dimension_lists.
// It returns two values.  The first result
// is a list of the coefficients.  The second
// is a list with all the incremented iterators.
// When we encounter a base_dimension that is
// missing from a dimension_list, we do not
// increment the iterator and we set the
// coefficient to zero.

template<bool has_dimension>
struct calculate_base_dimension_coefficients_func;

template<>
struct calculate_base_dimension_coefficients_func<true> {
    template<class T>
    struct apply {
        typedef typename T::item::value_type type;
        typedef typename T::next next;
    };
};

template<>
struct calculate_base_dimension_coefficients_func<false> {
    template<class T>
    struct apply {
        typedef static_rational<0> type;
        typedef T next;
    };
};

// begins_with_dimension returns true iff its first
// parameter is a valid iterator which yields its
// second parameter when dereferenced.

template<class Iterator>
struct begins_with_dimension {
    template<class Dim>
    struct apply : 
        boost::is_same<
            Dim,
            typename Iterator::item::tag_type
        > {};
};

template<>
struct begins_with_dimension<dimensionless_type> {
    template<class Dim>
    struct apply : mpl::false_ {};
};

template<int N>
struct calculate_base_dimension_coefficients_impl {
    template<class BaseUnitDimensions,class Dim,class T>
    struct apply {
        typedef typename calculate_base_dimension_coefficients_func<
            begins_with_dimension<typename BaseUnitDimensions::item>::template apply<
                Dim
            >::value
        >::template apply<
            typename BaseUnitDimensions::item
        > result;
        typedef typename calculate_base_dimension_coefficients_impl<N-1>::template apply<
            typename BaseUnitDimensions::next,
            Dim,
            list<typename result::type, T>
        > next_;
        typedef typename next_::type type;
        typedef list<typename result::next, typename next_::next> next;
    };
};

template<>
struct calculate_base_dimension_coefficients_impl<0> {
    template<class Begin, class BaseUnitDimensions, class T>
    struct apply {
        typedef T type;
        typedef dimensionless_type next;
    };
};

// add_zeroes pushs N zeroes onto the
// front of a list.
//
// list<rational> add_zeroes(list<rational> l, int N) {
//     if(N == 0) {
//         return(l);
//     } else {
//         return(push_front(add_zeroes(l, N-1), 0));
//     }
// }

template<int N>
struct add_zeroes_impl {
    // If you get an error here and your base units are
    // in fact linearly independent, please report it.
    BOOST_MPL_ASSERT_MSG((N > 0), base_units_are_probably_not_linearly_independent, (void));
    template<class T>
    struct apply {
        typedef list<
            static_rational<0>,
            typename add_zeroes_impl<N-1>::template apply<T>::type
        > type;
    };
};

template<>
struct add_zeroes_impl<0> {
    template<class T>
    struct apply {
        typedef T type;
    };
};

// expand_dimensions finds the exponents of
// a set of dimensions in a dimension_list.
// the second parameter is assumed to be
// a superset of the base_dimensions of
// the first parameter.
//
// list<rational> expand_dimensions(dimension_list, list<base_dimension>);

template<int N>
struct expand_dimensions {
    template<class Begin, class DimensionIterator>
    struct apply {
        typedef typename calculate_base_dimension_coefficients_func<
            begins_with_dimension<DimensionIterator>::template apply<typename Begin::item>::value
        >::template apply<DimensionIterator> result;
        typedef list<
            typename result::type,
            typename expand_dimensions<N-1>::template apply<typename Begin::next, typename result::next>::type
        > type;
    };
};

template<>
struct expand_dimensions<0> {
    template<class Begin, class DimensionIterator>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<int N>
struct create_unit_matrix {
    template<class Begin, class Dimensions>
    struct apply {
        typedef typename create_unit_matrix<N - 1>::template apply<typename Begin::next, Dimensions>::type next;
        typedef list<typename expand_dimensions<Dimensions::size::value>::template apply<Dimensions, typename Begin::item::dimension_type>::type, next> type;
    };
};

template<>
struct create_unit_matrix<0> {
    template<class Begin, class Dimensions>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<class T>
struct normalize_units {
    typedef typename find_base_dimensions<T>::type dimensions;
    typedef typename create_unit_matrix<(T::size::value)>::template apply<
        T,
        dimensions
    >::type matrix;
    typedef typename make_square_and_invert<matrix>::type type;
    BOOST_STATIC_CONSTEXPR long extra = (type::size::value) - (T::size::value);
};

// multiply_add_units computes M x V
// where M is a matrix and V is a horizontal
// vector
//
// list<rational> multiply_add_units(list<list<rational> >, list<rational>);

template<int N>
struct multiply_add_units_impl {
    template<class Begin1, class Begin2 ,class X>
    struct apply {
        typedef list<
            typename mpl::plus<
                typename mpl::times<
                    typename Begin2::item,
                    X
                >::type,
                typename Begin1::item
            >::type,
            typename multiply_add_units_impl<N-1>::template apply<
                typename Begin1::next,
                typename Begin2::next,
                X
            >::type
        > type;
    };
};

template<>
struct multiply_add_units_impl<0> {
    template<class Begin1, class Begin2 ,class X>
    struct apply {
        typedef dimensionless_type type;
    };
};

template<int N>
struct multiply_add_units {
    template<class Begin1, class Begin2>
    struct apply {
        typedef typename multiply_add_units_impl<
            (Begin2::item::size::value)
        >::template apply<
            typename multiply_add_units<N-1>::template apply<
                typename Begin1::next,
                typename Begin2::next
            >::type,
            typename Begin2::item,
            typename Begin1::item
        >::type type;
    };
};

template<>
struct multiply_add_units<1> {
    template<class Begin1, class Begin2>
    struct apply {
        typedef typename add_zeroes_impl<
            (Begin2::item::size::value)
        >::template apply<dimensionless_type>::type type1;
        typedef typename multiply_add_units_impl<
            (Begin2::item::size::value)
        >::template apply<
            type1,
            typename Begin2::item,
            typename Begin1::item
        >::type type;
    };
};


// strip_zeroes erases the first N elements of a list if
// they are all zero, otherwise returns inconsistent
//
// list strip_zeroes(list l, int N) {
//     if(N == 0) {
//         return(l);
//     } else if(l.front == 0) {
//         return(strip_zeroes(pop_front(l), N-1));
//     } else {
//         return(inconsistent);
//     }
// }

template<int N>
struct strip_zeroes_impl;

template<class T>
struct strip_zeroes_func {
    template<class L, int N>
    struct apply {
        typedef inconsistent type;
    };
};

template<>
struct strip_zeroes_func<static_rational<0> > {
    template<class L, int N>
    struct apply {
        typedef typename strip_zeroes_impl<N-1>::template apply<typename L::next>::type type;
    };
};

template<int N>
struct strip_zeroes_impl {
    template<class T>
    struct apply {
        typedef typename strip_zeroes_func<typename T::item>::template apply<T, N>::type type;
    };
};

template<>
struct strip_zeroes_impl<0> {
    template<class T>
    struct apply {
        typedef T type;
    };
};

// Given a list of base_units, computes the
// exponents of each base unit for a given
// dimension.
//
// list<rational> calculate_base_unit_exponents(list<base_unit> units, dimension_list dimensions);

template<class T>
struct is_base_dimension_unit {
    typedef mpl::false_ type;
    typedef void base_dimension_type;
};
template<class T>
struct is_base_dimension_unit<list<dim<T, static_rational<1> >, dimensionless_type> > {
    typedef mpl::true_ type;
    typedef T base_dimension_type;
};

template<int N>
struct is_simple_system_impl {
    template<class Begin, class Prev>
    struct apply {
        typedef is_base_dimension_unit<typename Begin::item::dimension_type> test;
        typedef mpl::and_<
            typename test::type,
            mpl::less<Prev, typename test::base_dimension_type>,
            typename is_simple_system_impl<N-1>::template apply<
                typename Begin::next,
                typename test::base_dimension_type
            >
        > type;
        BOOST_STATIC_CONSTEXPR bool value = (type::value);
    };
};

template<>
struct is_simple_system_impl<0> {
    template<class Begin, class Prev>
    struct apply : mpl::true_ {
    };
};

template<class T>
struct is_simple_system {
    typedef T Begin;
    typedef is_base_dimension_unit<typename Begin::item::dimension_type> test;
    typedef typename mpl::and_<
        typename test::type,
        typename is_simple_system_impl<
            T::size::value - 1
        >::template apply<
            typename Begin::next::type,
            typename test::base_dimension_type
        >
    >::type type;
    BOOST_STATIC_CONSTEXPR bool value = type::value;
};

template<bool>
struct calculate_base_unit_exponents_impl;

template<>
struct calculate_base_unit_exponents_impl<true> {
    template<class T, class Dimensions>
    struct apply {
        typedef typename expand_dimensions<(T::size::value)>::template apply<
            typename find_base_dimensions<T>::type,
            Dimensions
        >::type type;
    };
};

template<>
struct calculate_base_unit_exponents_impl<false> {
    template<class T, class Dimensions>
    struct apply {
        // find the units that correspond to each base dimension
        typedef normalize_units<T> base_solutions;
        // pad the dimension with zeroes so it can just be a
        // list of numbers, making the multiplication easy
        // e.g. if the arguments are list<pound, foot> and
        // list<mass,time^-2> then this step will
        // yield list<0,1,-2>
        typedef typename expand_dimensions<(base_solutions::dimensions::size::value)>::template apply<
            typename base_solutions::dimensions,
            Dimensions
        >::type dimensions;
        // take the unit corresponding to each base unit
        // multiply each of its exponents by the exponent
        // of the base_dimension in the result and sum.
        typedef typename multiply_add_units<dimensions::size::value>::template apply<
            dimensions,
            typename base_solutions::type
        >::type units;
        // Now, verify that the dummy units really
        // cancel out and remove them.
        typedef typename strip_zeroes_impl<base_solutions::extra>::template apply<units>::type type;
    };
};

template<class T, class Dimensions>
struct calculate_base_unit_exponents {
    typedef typename calculate_base_unit_exponents_impl<is_simple_system<T>::value>::template apply<T, Dimensions>::type type;
};

} // namespace detail

} // namespace units

} // namespace boost

#endif
