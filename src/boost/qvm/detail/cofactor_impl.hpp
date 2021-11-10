#ifndef BOOST_QVM_DETAIL_COFACTOR_IMPL_HPP_INCLUDED
#define BOOST_QVM_DETAIL_COFACTOR_IMPL_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/detail/determinant_impl.hpp>
#include <boost/qvm/mat_traits.hpp>
#include <boost/qvm/static_assert.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <class A>
    BOOST_QVM_INLINE_OPERATIONS
    typename deduce_mat<A>::type
    cofactor_impl( A const & a )
        {
        BOOST_QVM_STATIC_ASSERT(mat_traits<A>::rows==mat_traits<A>::cols);
        int const N=mat_traits<A>::rows;
        typedef typename mat_traits<A>::scalar_type T;
        T c[N-1][N-1];
        typedef typename deduce_mat<A>::type R;
        R b;
        for( int j=0; j!=N; ++j )
            {
            for( int i=0; i!=N; ++i )
                {
                int i1=0;
                for( int ii=0; ii!=N; ++ii )
                    {
                    if( ii==i )
                        continue;
                    int j1=0;
                    for( int jj=0; jj!=N; ++jj )
                        {
                        if( jj==j )
                            continue;
                        c[i1][j1] = mat_traits<A>::read_element_idx(ii,jj,a);
                        ++j1;
                        }
                    ++i1;
                    }
                T det = determinant_impl(c);
                if( (i+j)&1 )
                    det=-det;
                mat_traits<R>::write_element_idx(i,j,b) = det;
                }
            }
        return b;
        }
    }

} }

#endif
