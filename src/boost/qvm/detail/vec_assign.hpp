#ifndef BOOST_QVM_DETAIL_VEC_ASSIGN_HPP_INCLUDED
#define BOOST_QVM_DETAIL_VEC_ASSIGN_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/gen/vec_assign2.hpp>
#include <boost/qvm/gen/vec_assign3.hpp>
#include <boost/qvm/gen/vec_assign4.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <int D>
    struct
    assign_vv_defined
        {
        static bool const value=false;
        };

    template <int I,int N>
    struct
    copy_vector_elements
        {
        template <class A,class B>
        static
        void
        f( A & a, B const & b )
            {
            vec_traits<A>::template write_element<I>(a)=vec_traits<B>::template read_element<I>(b);
            copy_vector_elements<I+1,N>::f(a,b);
            }
        };

    template <int N>
    struct
    copy_vector_elements<N,N>
        {
        template <class A,class B>
        static
        void
        f( A &, B const & )
            {
            }
        };
    }

template <class A,class B>
inline
typename enable_if_c<
    is_vec<A>::value && is_vec<B>::value &&
    vec_traits<A>::dim==vec_traits<B>::dim &&
    !qvm_detail::assign_vv_defined<vec_traits<A>::dim>::value,
    A &>::type
assign( A & a, B const & b )
    {
    qvm_detail::copy_vector_elements<0,vec_traits<A>::dim>::f(a,b);
    return a;
    }

} }

#endif
