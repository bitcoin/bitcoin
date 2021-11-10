#ifndef BOOST_QVM_DETAIL_TRANSP_IMPL_HPP_INCLUDED
#define BOOST_QVM_DETAIL_TRANSP_IMPL_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/deduce_mat.hpp>
#include <boost/qvm/static_assert.hpp>
#include <boost/qvm/assert.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <class OriginalMatrix>
    class
    transposed_
        {
        transposed_( transposed_ const & );
        transposed_ & operator=( transposed_ const & );
        ~transposed_();

        public:

        template <class T>
        BOOST_QVM_INLINE_TRIVIAL
        transposed_ &
        operator=( T const & x )
            {
            assign(*this,x);
            return *this;
            }

        template <class R>
        BOOST_QVM_INLINE_TRIVIAL
        operator R() const
            {
            R r;
            assign(r,*this);
            return r;
            }
        };
    }

template <class OriginalMatrix>
struct
mat_traits< qvm_detail::transposed_<OriginalMatrix> >
    {
    typedef typename mat_traits<OriginalMatrix>::scalar_type scalar_type;
    typedef qvm_detail::transposed_<OriginalMatrix> this_matrix;
    static int const rows=mat_traits<OriginalMatrix>::cols;
    static int const cols=mat_traits<OriginalMatrix>::rows;

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_matrix const & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return mat_traits<OriginalMatrix>::template read_element<Col,Row>(reinterpret_cast<OriginalMatrix const &>(x));
        }

    template <int Row,int Col>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_matrix & x )
        {
        BOOST_QVM_STATIC_ASSERT(Row>=0);
        BOOST_QVM_STATIC_ASSERT(Row<rows);
        BOOST_QVM_STATIC_ASSERT(Col>=0);
        BOOST_QVM_STATIC_ASSERT(Col<cols);
        return mat_traits<OriginalMatrix>::template write_element<Col,Row>(reinterpret_cast<OriginalMatrix &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int row, int col, this_matrix const & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return mat_traits<OriginalMatrix>::read_element_idx(col,row,reinterpret_cast<OriginalMatrix const &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int row, int col, this_matrix & x )
        {
        BOOST_QVM_ASSERT(row>=0);
        BOOST_QVM_ASSERT(row<rows);
        BOOST_QVM_ASSERT(col>=0);
        BOOST_QVM_ASSERT(col<cols);
        return mat_traits<OriginalMatrix>::write_element_idx(col,row,reinterpret_cast<OriginalMatrix &>(x));
        }
    };

template <class OriginalMatrix,int R,int C>
struct
deduce_mat<qvm_detail::transposed_<OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

template <class OriginalMatrix,int R,int C>
struct
deduce_mat2<qvm_detail::transposed_<OriginalMatrix>,qvm_detail::transposed_<OriginalMatrix>,R,C>
    {
    typedef mat<typename mat_traits<OriginalMatrix>::scalar_type,R,C> type;
    };

} }

#endif
