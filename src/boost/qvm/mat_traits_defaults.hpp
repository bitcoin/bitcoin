#ifndef BOOST_QVM_MAT_TRAITS_DEFAULTS_HPP_INCLUDED
#define BOOST_QVM_MAT_TRAITS_DEFAULTS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/assert.hpp>

namespace boost { namespace qvm {

template <class>
struct mat_traits;

namespace
qvm_detail
    {
    template <int I,int N>
    struct
    matrix_w
        {
        template <class A>
        static
        BOOST_QVM_INLINE_CRITICAL
        typename mat_traits<A>::scalar_type &
        write_element_idx( int r, int c, A & a )
            {
            return (I/mat_traits<A>::cols)==r && (I%mat_traits<A>::cols)==c?
                mat_traits<A>::template write_element<I/mat_traits<A>::cols,I%mat_traits<A>::cols>(a) :
                matrix_w<I+1,N>::write_element_idx(r,c,a);
            }
        };

    template <int N>
    struct
    matrix_w<N,N>
        {
        template <class A>
        static
        BOOST_QVM_INLINE_TRIVIAL
        typename mat_traits<A>::scalar_type &
        write_element_idx( int, int, A & a )
            {
            BOOST_QVM_ASSERT(0);
            return mat_traits<A>::template write_element<0,0>(a);
            }
        };
    }

template <class MatType,class ScalarType,int Rows,int Cols>
struct
mat_traits_defaults
    {
    typedef MatType mat_type;
    typedef ScalarType scalar_type;
    static int const rows=Rows;
    static int const cols=Cols;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( mat_type const & x )
        {
        return mat_traits<mat_type>::template write_element<Row,Col>(const_cast<mat_type &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int r, int c, mat_type const & x )
        {
        return mat_traits<mat_type>::write_element_idx(r,c,const_cast<mat_type &>(x));
        }

    protected:

    static
    BOOST_QVM_INLINE_TRIVIAL
    scalar_type &
    write_element_idx( int r, int c, mat_type & m )
        {
        return qvm_detail::matrix_w<0,mat_traits<mat_type>::rows*mat_traits<mat_type>::cols>::write_element_idx(r,c,m);
        }
    };

} }

#endif
