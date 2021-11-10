#ifndef BOOST_QVM_DEDUCE_MAT_HPP_INCLUDED
#define BOOST_QVM_DEDUCE_MAT_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/deduce_scalar.hpp>
#include <boost/qvm/mat_traits.hpp>
#include <boost/qvm/static_assert.hpp>

namespace boost { namespace qvm {

template <class T,int Rows,int Cols>
struct mat;

namespace
qvm_detail
    {
    template <class M,int R,int C,class S,
        int MR=mat_traits<M>::rows,
        int MC=mat_traits<M>::cols,
        class MS=typename mat_traits<M>::scalar_type>
    struct
    deduce_m_default
        {
        BOOST_QVM_STATIC_ASSERT(is_mat<M>::value);
        typedef mat<typename mat_traits<M>::scalar_type,R,C> type;
        };

    template <class M,int R,int C,class S>
    struct
    deduce_m_default<M,R,C,S,R,C,S>
        {
        BOOST_QVM_STATIC_ASSERT(is_mat<M>::value);
        typedef M type;
        };
    }

template <class M,int R=mat_traits<M>::rows,int C=mat_traits<M>::cols,class S=typename mat_traits<M>::scalar_type>
struct
deduce_mat
    {
    BOOST_QVM_STATIC_ASSERT(is_mat<M>::value);
    typedef typename qvm_detail::deduce_m_default<M,R,C,S>::type type;
    };

namespace
qvm_detail
    {
    template <class A,class B,int R,int C,class S,
        bool IsScalarA=is_scalar<A>::value,
        bool IsScalarB=is_scalar<B>::value>
    struct
    deduce_m2_default
        {
        typedef mat<S,R,C> type;
        };

    template <class M,int R,int C,class S>
    struct
    deduce_m2_default<M,M,R,C,S,false,false>
        {
        BOOST_QVM_STATIC_ASSERT(is_mat<M>::value);
        typedef M type;
        };

    template <class A,class B,int R,int C,class S>
    struct
    deduce_m2_default<A,B,R,C,S,false,true>
        {
        BOOST_QVM_STATIC_ASSERT(is_mat<A>::value);
        typedef typename deduce_mat<A,R,C,S>::type type;
        };

    template <class A,class B,int R,int C,class S>
    struct
    deduce_m2_default<A,B,R,C,S,true,false>
        {
        BOOST_QVM_STATIC_ASSERT(is_mat<B>::value);
        typedef typename deduce_mat<B,R,C,S>::type type;
        };
    }

template <class A,class B,int R,int C,class S=typename deduce_scalar_detail::deduce_scalar_impl<typename scalar<A>::type,typename scalar<B>::type>::type>
struct
deduce_mat2
    {
    BOOST_QVM_STATIC_ASSERT(is_mat<A>::value || is_mat<B>::value);
    typedef typename qvm_detail::deduce_m2_default<A,B,R,C,S>::type type;
    };

} }

#endif
