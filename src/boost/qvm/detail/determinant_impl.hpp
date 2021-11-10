#ifndef BOOST_QVM_DETAIL_DETERMINANT_IMPL_HPP_INCLUDED
#define BOOST_QVM_DETAIL_DETERMINANT_IMPL_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/mat_traits_array.hpp>
#include <boost/qvm/static_assert.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <int N>
    struct
    det_size
        {
        };

    template <class M>
    BOOST_QVM_INLINE_TRIVIAL
    typename mat_traits<M>::scalar_type
    determinant_impl_( M const & a, det_size<2> )
        {
        return
            mat_traits<M>::template read_element<0,0>(a) * mat_traits<M>::template read_element<1,1>(a) -
            mat_traits<M>::template read_element<1,0>(a) * mat_traits<M>::template read_element<0,1>(a);
        }

    template <class M,int N>
    BOOST_QVM_INLINE_RECURSION
    typename mat_traits<M>::scalar_type
    determinant_impl_( M const & a, det_size<N> )
        {
        typedef typename mat_traits<M>::scalar_type T;
        T m[N-1][N-1];
        T det=T(0);
        for( int j1=0; j1!=N; ++j1 )
            {
            for( int i=1; i!=N; ++i )
                {
                int j2 = 0;
                for( int j=0; j!=N; ++j )
                    {
                    if( j==j1 )
                        continue;
                    m[i-1][j2] = mat_traits<M>::read_element_idx(i,j,a);
                    ++j2;
                    }
                }
            T d=determinant_impl_(m,det_size<N-1>());
            if( j1&1 )
                d=-d;
            det += mat_traits<M>::read_element_idx(0,j1,a) * d;
            }
        return det;
        }

    template <class M>
    BOOST_QVM_INLINE_TRIVIAL
    typename mat_traits<M>::scalar_type
    determinant_impl( M const & a )
        {
        BOOST_QVM_STATIC_ASSERT(mat_traits<M>::rows==mat_traits<M>::cols);
        return determinant_impl_(a,det_size<mat_traits<M>::rows>());
        }
    }

} }

#endif
