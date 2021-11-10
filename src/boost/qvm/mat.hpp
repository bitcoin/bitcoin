#ifndef BOOST_QVM_MAT_HPP_INCLUDED
#define BOOST_QVM_MAT_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/detail/mat_assign.hpp>
#include <boost/qvm/assert.hpp>
#include <boost/qvm/static_assert.hpp>

namespace boost { namespace qvm {

template <class T,int Rows,int Cols>
struct
mat
    {
    T a[Rows][Cols];
    template <class R>
    operator R() const
        {
        R r;
        assign(r,*this);
        return r;
        }
    };

template <class M>
struct mat_traits;

template <class T,int Rows,int Cols>
struct
mat_traits< mat<T,Rows,Cols> >
    {
    typedef mat<T,Rows,Cols> this_matrix;
    typedef T scalar_type;
    static int const rows=Rows;
    static int const cols=Cols;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<Rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<Cols);
        return x.a[Row][Col];
        }

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_matrix & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<Rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<Cols);
        return x.a[Row][Col];
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<Rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<Cols);
        return x.a[row][col];
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int row, int col, this_matrix & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<Rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<Cols);
        return x.a[row][col];
        }
    };

} }

#endif
