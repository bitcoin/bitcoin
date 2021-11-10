#ifndef BOOST_QVM_VEC_MAT_OPERATIONS_HPP_INCLUDED
#define BOOST_QVM_VEC_MAT_OPERATIONS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/vec_mat_operations2.hpp>
#include <boost/qvm/vec_mat_operations3.hpp>
#include <boost/qvm/vec_mat_operations4.hpp>

namespace boost { namespace qvm {

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    mul_mv_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_mat<A>::value && is_vec<B>::value &&
    mat_traits<A>::cols==vec_traits<B>::dim &&
    !qvm_detail::mul_mv_defined<mat_traits<A>::rows,mat_traits<A>::cols>::value,
    deduce_vec2<A,B,mat_traits<A>::rows> >::type
operator*( A const & a, B const & b )
    {
    typedef typename deduce_vec2<A,B,mat_traits<A>::rows>::type R;
    R r;
    for( int i=0; i<mat_traits<A>::rows; ++i )
        {
        typedef typename vec_traits<R>::scalar_type Tr;
        Tr x(scalar_traits<Tr>::value(0));
        for( int j=0; j<mat_traits<A>::cols; ++j )
            x += mat_traits<A>::read_element_idx(i,j,a)*vec_traits<B>::read_element_idx(j,b);
        vec_traits<R>::write_element_idx(i,r) = x;
        }
    return r;
    }

namespace
qvm_detail
    {
    template <int M,int N>
    struct
    mul_vm_defined
        {
        static bool const value=false;
        };
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_vec<A>::value && is_mat<B>::value &&
    vec_traits<A>::dim==mat_traits<B>::rows &&
    !qvm_detail::mul_vm_defined<mat_traits<B>::rows,mat_traits<B>::cols>::value,
    deduce_vec2<A,B,mat_traits<B>::cols> >::type
operator*( A const & a, B const & b )
    {
    typedef typename deduce_vec2<A,B,mat_traits<B>::cols>::type R;
    R r;
    for( int i=0; i<mat_traits<B>::cols; ++i )
        {
        typedef typename vec_traits<R>::scalar_type Tr;
        Tr x(scalar_traits<Tr>::value(0));
        for( int j=0; j<mat_traits<B>::rows; ++j )
            x += vec_traits<A>::read_element_idx(j,a)*mat_traits<B>::read_element_idx(j,i,b);
        vec_traits<R>::write_element_idx(i,r) = x;
        }
    return r;
    }

////////////////////////////////////////////////

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    mat_traits<A>::rows==4 && mat_traits<A>::cols==4 &&
    vec_traits<B>::dim==3,
    deduce_vec2<A,B,3> >::type
transform_point( A const & a, B const & b )
    {
    typedef typename mat_traits<A>::scalar_type Ta;
    typedef typename vec_traits<B>::scalar_type Tb;
    Ta const a00 = mat_traits<A>::template read_element<0,0>(a);
    Ta const a01 = mat_traits<A>::template read_element<0,1>(a);
    Ta const a02 = mat_traits<A>::template read_element<0,2>(a);
    Ta const a03 = mat_traits<A>::template read_element<0,3>(a);
    Ta const a10 = mat_traits<A>::template read_element<1,0>(a);
    Ta const a11 = mat_traits<A>::template read_element<1,1>(a);
    Ta const a12 = mat_traits<A>::template read_element<1,2>(a);
    Ta const a13 = mat_traits<A>::template read_element<1,3>(a);
    Ta const a20 = mat_traits<A>::template read_element<2,0>(a);
    Ta const a21 = mat_traits<A>::template read_element<2,1>(a);
    Ta const a22 = mat_traits<A>::template read_element<2,2>(a);
    Ta const a23 = mat_traits<A>::template read_element<2,3>(a);
    Tb const b0 = vec_traits<B>::template read_element<0>(b);
    Tb const b1 = vec_traits<B>::template read_element<1>(b);
    Tb const b2 = vec_traits<B>::template read_element<2>(b);
    typedef typename deduce_vec2<A,B,3>::type R;
    BOOST_QVM_STATIC_ASSERT(vec_traits<R>::dim==3);
    R r;
    vec_traits<R>::template write_element<0>(r)=a00*b0+a01*b1+a02*b2+a03;
    vec_traits<R>::template write_element<1>(r)=a10*b0+a11*b1+a12*b2+a13;
    vec_traits<R>::template write_element<2>(r)=a20*b0+a21*b1+a22*b2+a23;
    return r;
    }

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    mat_traits<A>::rows==4 && mat_traits<A>::cols==4 &&
    vec_traits<B>::dim==3,
    deduce_vec2<A,B,3> >::type
transform_vector( A const & a, B const & b )
    {
    typedef typename mat_traits<A>::scalar_type Ta;
    typedef typename vec_traits<B>::scalar_type Tb;
    Ta const a00 = mat_traits<A>::template read_element<0,0>(a);
    Ta const a01 = mat_traits<A>::template read_element<0,1>(a);
    Ta const a02 = mat_traits<A>::template read_element<0,2>(a);
    Ta const a10 = mat_traits<A>::template read_element<1,0>(a);
    Ta const a11 = mat_traits<A>::template read_element<1,1>(a);
    Ta const a12 = mat_traits<A>::template read_element<1,2>(a);
    Ta const a20 = mat_traits<A>::template read_element<2,0>(a);
    Ta const a21 = mat_traits<A>::template read_element<2,1>(a);
    Ta const a22 = mat_traits<A>::template read_element<2,2>(a);
    Tb const b0 = vec_traits<B>::template read_element<0>(b);
    Tb const b1 = vec_traits<B>::template read_element<1>(b);
    Tb const b2 = vec_traits<B>::template read_element<2>(b);
    typedef typename deduce_vec2<A,B,3>::type R;
    BOOST_QVM_STATIC_ASSERT(vec_traits<R>::dim==3);
    R r;
    vec_traits<R>::template write_element<0>(r)=a00*b0+a01*b1+a02*b2;
    vec_traits<R>::template write_element<1>(r)=a10*b0+a11*b1+a12*b2;
    vec_traits<R>::template write_element<2>(r)=a20*b0+a21*b1+a22*b2;
    return r;
    }

////////////////////////////////////////////////

namespace
sfinae
    {
    using ::boost::qvm::operator*;
    using ::boost::qvm::transform_point;
    using ::boost::qvm::transform_vector;
    }

} }

#endif
